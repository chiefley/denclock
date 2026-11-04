#include <assert.h>
#include <iostream>
#include "../lora-gateway/src/main.cpp"

void commands(const std::string& text) {
    Serial.input = text;
    while (Serial.available()) loop();
}
void wifiEvents() {
    WiFiEventInfo_t info{};
    info.wifi_sta_disconnected.reason = 7;
    onWiFiEvent(ARDUINO_EVENT_WIFI_STA_CONNECTED, info);
    onWiFiEvent(ARDUINO_EVENT_WIFI_STA_GOT_IP, info);
    onWiFiEvent(ARDUINO_EVENT_WIFI_STA_DISCONNECTED, info);
}
int main() {
    GatewayWiFi::Credentials credentials;
    strcpy(credentials.ssid, "Test network");
    strcpy(credentials.password, "test-password");
    assert(GatewayWiFi::save(credentials));
    setup();
    Serial.output.clear();
    wifiEvents();
    assert(Serial.output.find("Wi-Fi associated") != std::string::npos);
    assert(Serial.output.find("Reason: 7") != std::string::npos);
    WiFi.statusValue = WL_CONNECTED;
    WiFi.resolveOk = true;
    strcpy(activeConfig.host, "service.example");
    maintainConnection();
    assert(service.connected());

    commands("config\n");
    assert(console.active && Serial.output.substr(Serial.output.size()-2) == "> ");
    Serial.output.clear();
    // An incomplete command survives asynchronous events and multiple loop passes.
    commands("sh");
    assert(Serial.output == "sh");
    wifiEvents();
    SensorReport::encode(radio.packet, 2, 0, 2200, 5000, 4000);
    const size_t bytes = service.frames.size();
    received = true; loop();
    assert(service.frames.size() > bytes && diagnostics.accepted == 1);
    assert(!display.lines.empty());
    radio.packet[0] = 0; // Invalid report still updates rejection diagnostics.
    const unsigned rejected = diagnostics.rejected;
    received = true; loop();
    assert(diagnostics.rejected == rejected + 1);
    radio.readResult = -1;
    received = true; loop();
    assert(diagnostics.rejected == rejected + 2);
    radio.readResult = 0;
    service.writeOk = false;
    const uint8_t frame[] = {1, 2, 3};
    assert(!sendFrame(frame, sizeof(frame)) && !service.connected());
    service.writeOk = true;
    updateConnectivity();
    WiFi.resolveOk = false; fakeMillis += RetryMs;
    const unsigned attempts = WiFi.resolutions;
    maintainConnection();
    assert(WiFi.resolutions == attempts + 1 && !service.connected());
    strcpy(activeConfig.host, "missing.local");
    fakeMillis += RetryMs; maintainConnection();
    assert(mdnsInitializations == 1); // mDNS initialization failure stays silent.
    strcpy(activeConfig.host, "service.example");
    WiFi.resolveOk = true; service.connectOk = false;
    fakeMillis += RetryMs; maintainConnection();
    assert(!service.connected());
    service.connectOk = true; fakeMillis += RetryMs; maintainConnection();
    assert(service.connected());
    WiFi.statusValue = 0; maintainConnection();
    assert(!service.connected());
    provisioning.tick(); fakeMillis += RetryMs; provisioning.tick();
    assert(WiFi.reconnects > 0);
    provisioning.start();
    assert(provisioning.active);
    provisioning.stop();
    assert(!provisioning.active);
    assert(Serial.output == "sh"); // No diagnostics, including provisioning events.
    commands("ow\nhelp\nset host invalid/name\nsave\n");
    assert(Serial.output.find("show\r\n") != std::string::npos);
    assert(Serial.output.find("Service Host:") != std::string::npos);
    assert(Serial.output.find("Command reference") != std::string::npos);
    assert(Serial.output.find("Invalid command or value") != std::string::npos);
    assert(Serial.output.find("Configuration saved") != std::string::npos);
    failSave = true; commands("save\n");
    assert(Serial.output.find("Save failed") != std::string::npos);
    commands("defaults\n"); assert(console.active);
    commands("exit\n"); assert(!console.active);
    Serial.output.clear(); loop();
    assert(Serial.output.empty()); // Nothing buffered for replay.
    wifiEvents(); assert(Serial.output.find("Reason: 7") != std::string::npos);

    commands("config\n"); Serial.output.clear();
    wifiEvents(); assert(Serial.output.empty());
    fakeMillis += 300000; loop();
    assert(!console.active && Serial.output.find("Configuration timeout") != std::string::npos);
    Serial.output.clear(); wifiEvents();
    assert(Serial.output.find("Reason: 7") != std::string::npos);
    std::cout << "Gateway serial suppression tests passed\n";
}
