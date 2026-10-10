#include <Arduino.h>
#include <WiFi.h>
#include <SSD1306Wire.h>
#include "config.h"
#include "gateway_protocol.h"
#include "gateway_diagnostics.h"
#include "../../common/radio_config.h"

SX1262 radio = new Module(RadioNss, RadioDio1, RadioReset, RadioBusy);
WiFiClient service;
volatile bool received = false;
constexpr uint32_t RetryMs = 5000;
uint32_t lastWifiAttempt = 0, lastTcpAttempt = 0;
bool networkConfigured = false;
SSD1306Wire display(0x3C, 17, 18);
GatewayDiagnostics diagnostics;
constexpr uint32_t WifiSignalRefreshMs = 5000;
uint32_t lastWifiSignalRead = 0;

void updateConnectivity() {
    const uint32_t now = millis();
    const bool wifi = WiFi.status() == WL_CONNECTED;
    const bool server = service.connected();
    int32_t signal = diagnostics.wifiRssi;
    if (wifi && (!diagnostics.wifiConnected ||
                 uint32_t(now - lastWifiSignalRead) >= WifiSignalRefreshMs)) {
        signal = WiFi.RSSI();
        lastWifiSignalRead = now;
    }
    if (diagnostics.serverConnected && !server)
        Serial.println("LoRa Service TCP disconnected");
    diagnostics.connectivity(wifi, server, signal);
}

void updateDisplay() {
    updateConnectivity();
    diagnostics.tick(millis());
    if (!diagnostics.dirty) return;
    diagnostics.dirty = false;
    display.clear();
    display.setColor(WHITE);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    if (diagnostics.errorVisible) {
        display.fillRect(0, 0, 128, 64);
        display.setColor(BLACK);
        display.setFont(ArialMT_Plain_24);
        display.drawString(64, 3, "PACKET");
        display.drawString(64, 32, "ERROR");
    } else {
        char line[48];
        display.setFont(ArialMT_Plain_10);
        if (diagnostics.node) snprintf(line, sizeof(line), "NODE %02u", diagnostics.node);
        else snprintf(line, sizeof(line), "NODE --");
        display.drawString(64, 0, line);
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        // Small-font ink occupies y+3..y+9. Nine-pixel row spacing fits
        // all seven rows in 64 pixels, including full ten-digit counters.
        if (diagnostics.wifiConnected)
            snprintf(line, sizeof(line), "WiFi: OK  %ld dBm",
                     static_cast<long>(diagnostics.wifiRssi));
        else snprintf(line, sizeof(line), "WiFi: DOWN");
        display.drawString(0, 9, line);
        display.drawString(0, 18, diagnostics.serverConnected ? "Server: OK" : "Server: DOWN");
        if (diagnostics.node)
            snprintf(line, sizeof(line), "RSSI %.0fdBm  SNR %+.0fdB",
                     diagnostics.rssi, diagnostics.snr);
        else snprintf(line, sizeof(line), "RSSI --dBm  SNR --dB");
        display.drawString(0, 27, line);
        if (diagnostics.hasBattery)
            snprintf(line, sizeof(line), "BAT %.2fV", diagnostics.battery / 1000.0);
        else snprintf(line, sizeof(line), "BAT N/A");
        display.drawString(0, 36, line);
        snprintf(line, sizeof(line), "RX %lu", static_cast<unsigned long>(diagnostics.accepted));
        display.drawString(0, 45, line);
        snprintf(line, sizeof(line), "ERR %lu", static_cast<unsigned long>(diagnostics.rejected));
        display.drawString(0, 54, line);
    }
    display.display();
}

void IRAM_ATTR packetReceived() { received = true; }

bool sendFrame(const uint8_t* frame, size_t length) {
    // Partial writes must finish this frame before another frame can be sent.
    size_t sent = 0;
    while (sent < length && service.connected()) {
        size_t count = service.write(frame + sent, length - sent);
        if (count == 0) break;
        sent += count;
    }
    if (sent == length) return true;
    Serial.println("TCP write failed; dropping frame and reconnecting");
    service.stop(); // Never append a new frame after an incomplete frame.
    lastTcpAttempt = millis();
    return false;
}

void maintainConnection() {
    if (!networkConfigured) return;
    const uint32_t now = millis();
    if (WiFi.status() != WL_CONNECTED) {
        if (service.connected()) Serial.println("Wi-Fi lost; closing TCP connection");
        service.stop();
        if (uint32_t(now - lastWifiAttempt) >= RetryMs) {
            lastWifiAttempt = now;
            Serial.println("Connecting to Wi-Fi...");
            WiFi.reconnect();
        }
        return;
    }
    if (service.connected()) return;
    if (uint32_t(now - lastTcpAttempt) < RetryMs) return;
    lastTcpAttempt = now;
    service.stop();
    Serial.printf("Connecting to LoRa Service %s:%u\n", LORA_SERVICE_HOST, LORA_SERVICE_PORT);
    if (!service.connect(LORA_SERVICE_HOST, LORA_SERVICE_PORT, 1000)) return;
    service.setNoDelay(true);
    service.setTimeout(1); // WiFiClient uses seconds for socket I/O timeout.
    uint8_t hello[4];
    GatewayProtocol::encodeHello(hello, GatewayId);
    if (sendFrame(hello, sizeof(hello))) Serial.println("Gateway 1 connected; HELLO sent");
}

void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Wi-Fi associated with access point");
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("Wi-Fi connected. IP: ");
            Serial.println(WiFi.localIP());
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.printf("Wi-Fi disconnected. Reason: %d\n",
                          info.wifi_sta_disconnected.reason);
            break;

        default:
            break;
    }
}

void scanWiFi() {
    Serial.println("Scanning Wi-Fi networks...");
    int count = WiFi.scanNetworks();

    for (int i = 0; i < count; i++) {
        Serial.printf(
            "SSID: %s  RSSI: %d  Channel: %d  Auth: %d  BSSID: %s\n",
            WiFi.SSID(i).c_str(),
            WiFi.RSSI(i),
            WiFi.channel(i),
            static_cast<int>(WiFi.encryptionType(i)),
            WiFi.BSSIDstr(i).c_str()
        );
    }

    WiFi.scanDelete();
}

void startRadioReceive() {
    int16_t state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("Radio receive start failed: %d; reset required\n", state);
        while (true) delay(1000);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, LOW);
    delay(50);
    pinMode(21, OUTPUT);
    digitalWrite(21, LOW);
    delay(20);
    digitalWrite(21, HIGH);
    delay(20);
    display.init();
    display.flipScreenVertically();
    updateDisplay();
    int16_t state = initializeRadio(radio);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("Radio initialization failed: %d\n", state);
        while (true) delay(1000);
    }
    radio.setPacketReceivedAction(packetReceived);
    startRadioReceive();
    networkConfigured = GATEWAY_WIFI_SSID[0] != '\0' && LORA_SERVICE_HOST[0] != '\0';
    if (networkConfigured) {
        WiFi.onEvent(onWiFiEvent);
        WiFi.mode(WIFI_STA);
        scanWiFi();
        WiFi.setAutoReconnect(true);
        WiFi.begin(GATEWAY_WIFI_SSID, GATEWAY_WIFI_PASSWORD);
        lastWifiAttempt = millis();
        lastTcpAttempt = millis() - RetryMs;
    } else {
        Serial.println("Configure include/config.local.h; received packets will be discarded");
    }
    Serial.println("Gateway transport: 915 MHz, sync 0xD3C7, CRC on");
}

void loop() {
    // Handle any pending reception before a potentially blocking TCP reconnect.
    if (received) {
        radio.standby();
        received = false;
        uint8_t packet[GatewayProtocol::MaxLoRaLength];
        const size_t length = radio.getPacketLength();
        int16_t state = radio.readData(packet, length);
        const float rssi = radio.getRSSI(), snr = radio.getSNR();
        // Re-arm reception before OLED I2C or TCP work. Metadata and payload
        // above belong to the packet just read, not a subsequent reception.
        startRadioReceive();
        if (state != RADIOLIB_ERR_NONE) {
            diagnostics.reject("radio packet failure", state, millis());
            Serial.printf("RX discarded: radio error %d\n", state);
        } else if (!diagnostics.observe(packet, length, rssi, snr, millis())) {
            Serial.printf("OLED report rejected: %s\n", diagnostics.lastError);
        }
        updateDisplay();
        // OLED protocol validation is observational: preserve raw forwarding.
        if (state == RADIOLIB_ERR_NONE) {
            if (WiFi.status() != WL_CONNECTED || !service.connected()) {
                Serial.println("RX discarded: service disconnected");
            } else {
                uint8_t frame[GatewayProtocol::MaxFrameLength];
                size_t frameLength = GatewayProtocol::encodeRx(
                    frame, packet, length, rssi, snr);
                if (frameLength == 0) {
                    Serial.println("RX discarded: radio metadata outside wire range");
                } else if (sendFrame(frame, frameLength)) {
                    Serial.printf("RX_PACKET forwarded: %u raw LoRa bytes\n", unsigned(length));
                }
            }
        }
    }
    updateDisplay();
    maintainConnection();
    delay(1);
}
