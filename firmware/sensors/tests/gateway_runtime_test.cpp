#include <assert.h>
#include <iostream>
#include "../lora-gateway/src/main.cpp"
void commands(const char* text) {
    Serial.input = text;
    while (Serial.available()) loop();
}
int main() {
    GatewayWiFi::Credentials credentials;
    strcpy(credentials.ssid, "Test network"); strcpy(credentials.password, "test-password");
    assert(GatewayWiFi::save(credentials));
    setup();
    assert(!provisioning.active && WiFi.stationSsid == "Test network");
    WiFi.statusValue = WL_CONNECTED;
    assert(radio.frequency == 915.0f && radio.syncWord == 0xDC && radio.controlBits == 0x37);
    strcpy(activeConfig.host, "Original.Example");
    maintainConnection();
    assert(WiFi.resolutions == 1 && service.connects == 0);
    assert(Serial.output.find("hostname resolution failed") != std::string::npos);
    fakeMillis += RetryMs - 1; maintainConnection(); assert(WiFi.resolutions == 1);
    ++fakeMillis; WiFi.resolveOk = true; maintainConnection();
    assert(WiFi.resolutions == 2 && service.connects == 1 && service.frames.size() == 4);
    assert(WiFi.lastHost == "Original.Example");
    assert(Serial.output.find("resolved to 1.2.3.4") != std::string::npos);
    const size_t logLength = Serial.output.size();
    maintainConnection();
    assert(Serial.output.size() == logLength && WiFi.resolutions == 2);
    const std::string hello = service.frames;
    assert(hello == std::string("\x01\x00\x01\x01", 4) && GatewayId == 1);
    assert(radio.power == 10);
    commands("config\nset host Staged.local\nset frequency 916500\nset sync ABCD\nsave\n");
    assert(console.active && !strcmp(activeConfig.host, "Original.Example"));
    assert(radio.frequency == 915.0f && service.connected());
    // Exercise the real loop and packet forwarding while editing.
    SensorReport::encode(radio.packet, 2, 0, 2200, 5000, 4000);
    received = true;
    unsigned starts = radio.receiveStarts;
    loop();
    assert(radio.receiveStarts == starts + 1 && diagnostics.accepted == 1);
    assert(service.frames.size() > hello.size());
    assert(service.frames.find(std::string(reinterpret_cast<char*>(radio.packet), sizeof(radio.packet))) != std::string::npos);
    service.stop(); fakeMillis += RetryMs;
    WiFi.resolvedAddress = 0x05060708;
    maintainConnection();
    assert(WiFi.resolutions == 3 && WiFi.lastHost == "Original.Example");
    assert(service.lastAddress == 0x05060708); // Resolve again on reconnect.
    strcpy(activeConfig.host, "Windows-Host.LoCaL.");
    service.stop(); fakeMillis += RetryMs;
    const unsigned dnsResolutions = WiFi.resolutions, connects = service.connects;
    maintainConnection(); // Initialization failure must be retried, without DNS fallback.
    assert(mdnsInitializations == 1 && mdnsQueries == 0);
    assert(Serial.output.find("mDNS client initialization failed") != std::string::npos);
    fakeMillis += RetryMs; mdnsInitResult = ESP_OK;
    maintainConnection(); // Missing name: no TCP connection.
    assert(mdnsInitializations == 2 && mdnsQueries == 1 && service.connects == connects);
    assert(mdnsLastHost == "Windows-Host");
    fakeMillis += RetryMs - 1; maintainConnection(); assert(mdnsQueries == 1);
    ++fakeMillis; mdnsQueryResult = ESP_OK; mdnsAddress = 0;
    maintainConnection(); // A zero address is never used.
    assert(mdnsQueries == 2 && service.connects == connects);
    fakeMillis += RetryMs; mdnsAddress = 0x05060708; service.connectOk = false;
    Serial.output.clear(); maintainConnection();
    assert(mdnsQueries == 3 && service.connects == connects + 1);
    assert(Serial.output.find("resolved to 5.6.7.8") != std::string::npos);
    assert(Serial.output.find("TCP connection attempt failed") != std::string::npos);
    assert(Serial.output.find("HELLO sent") == std::string::npos);
    fakeMillis += RetryMs; service.connectOk = true; mdnsAddress = 0x090A0B0C;
    Serial.output.clear(); maintainConnection();
    assert(service.connected() && mdnsQueries == 4 && mdnsInitializations == 2);
    assert(service.lastAddress == 0x090A0B0C); // Fresh mDNS query on reconnect.
    assert(WiFi.resolutions == dnsResolutions); // .local never uses ordinary DNS.
    assert(Serial.output.find("connected; HELLO sent") != std::string::npos);
    assert(service.frames.substr(service.frames.size() - 4) == hello);
    strcpy(activeConfig.host, "192.168.50.90");
    service.stop(); fakeMillis += RetryMs;
    maintainConnection();
    assert(WiFi.resolutions == dnsResolutions && mdnsQueries == 4);
    assert(service.lastAddress == 0xC0A8325A);
    // Even sustained serial input consumes only 64 characters per loop,
    // while a pending radio packet is forwarded first.
    Serial.input = std::string(200, 'x');
    received = true;
    size_t frameBytes = service.frames.size();
    loop();
    assert(Serial.input.size() == 136 && service.frames.size() > frameBytes);
    Serial.input.clear();
    WiFi.statusValue = 0; fakeMillis += RetryMs;
    maintainConnection(); assert(!service.connected());
    provisioning.tick(); fakeMillis += RetryMs; provisioning.tick();
    assert(WiFi.reconnects == 1);
    WiFi.statusValue = WL_CONNECTED;
    setup(); // Simulate startup loading the saved snapshot, not unsaved RAM.
    assert(!strcmp(activeConfig.host, "Staged.local"));
    assert(radio.frequency == 916.5f && radio.power == 10);
    assert(radio.syncWord == 0xAC && radio.controlBits == 0xBD);
    assert(!console.active);
    // Provisioning keeps radio/console servicing alive without a packet backlog.
    provisioning.start();
    assert(provisioning.active);
    Serial.input = std::string(200, 'x');
    received = true;
    const unsigned accepted = diagnostics.accepted;
    const size_t bytesBeforeSetup = service.frames.size();
    loop();
    assert(diagnostics.accepted == accepted + 1 && Serial.input.size() == 136);
    assert(display.lines.size() == 6 && display.lines[0] == "SETUP");
    assert(display.lines[1] + display.lines[2] == provisioning.ssid);
    assert(display.lines[4] == provisioning.password);
    assert(service.frames.size() == bytesBeforeSetup && !service.connected());
    Serial.input.clear();
    provisioning.candidate = credentials;
    provisioning.testing = true; provisioning.attemptAt = millis();
    WiFi.statusValue = WL_CONNECTED;
    loop(); assert(provisioning.success && provisioning.active);
    fakeMillis += 60000; loop();
    assert(!provisioning.active && service.connected());
    assert(mdnsLastHost == "Staged");
    assert(service.frames.substr(service.frames.size() - 4) == hello);
    received = true; const size_t beforeForward = service.frames.size(); loop();
    assert(service.frames.size() > beforeForward);
    std::cout << "Gateway runtime tests passed\n";
}
