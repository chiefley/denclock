#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "gateway_protocol.h"
#include "../../common/radio_config.h"

SX1262 radio = new Module(RadioNss, RadioDio1, RadioReset, RadioBusy);
WiFiClient service;
volatile bool received = false;
constexpr uint32_t RetryMs = 5000;
uint32_t lastWifiAttempt = 0, lastTcpAttempt = 0;
bool networkConfigured = false;

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
    int16_t state = initializeRadio(radio);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("Radio initialization failed: %d\n", state);
        while (true) delay(1000);
    }
    radio.setPacketReceivedAction(packetReceived);
    startRadioReceive();
    networkConfigured = GATEWAY_WIFI_SSID[0] != '\0' && LORA_SERVICE_HOST[0] != '\0';
    if (networkConfigured) {
        WiFi.mode(WIFI_STA);
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
        if (state != RADIOLIB_ERR_NONE) {
            Serial.printf("RX discarded: radio error %d\n", state);
        } else if (WiFi.status() != WL_CONNECTED || !service.connected()) {
            Serial.println("RX discarded: service disconnected");
        } else {
            uint8_t frame[GatewayProtocol::MaxFrameLength];
            size_t frameLength = GatewayProtocol::encodeRx(
                frame, packet, length, radio.getRSSI(), radio.getSNR());
            if (frameLength == 0) {
                Serial.println("RX discarded: radio metadata outside wire range");
            } else if (sendFrame(frame, frameLength)) {
                Serial.printf("RX_PACKET forwarded: %u raw LoRa bytes\n", unsigned(length));
            }
        }
        startRadioReceive();
    }
    maintainConnection();
    delay(1);
}
