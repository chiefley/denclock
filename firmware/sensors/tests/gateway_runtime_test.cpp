#include <assert.h>
#include <iostream>
#include "../lora-gateway/src/main.cpp"
void commands(const char* text) {
    Serial.input = text;
    while (Serial.available()) loop();
}
int main() {
    setup();
    assert(radio.frequency == 915.0f && radio.syncWord == 0xDC && radio.controlBits == 0x37);
    strcpy(activeConfig.host, "Original.Example");
    maintainConnection();
    assert(WiFi.resolutions == 1 && service.connects == 0);
    assert(Serial.output.find("hostname resolution failed") != std::string::npos);
    fakeMillis += RetryMs - 1; maintainConnection(); assert(WiFi.resolutions == 1);
    ++fakeMillis; WiFi.resolveOk = true; maintainConnection();
    assert(WiFi.resolutions == 2 && service.connects == 1 && service.frames.size() == 4);
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
    strcpy(activeConfig.host, "Windows-Host.LoCaL");
    service.stop(); fakeMillis += RetryMs; MDNS.startOk = false;
    maintainConnection(); assert(MDNS.starts == 1 && MDNS.queries == 0);
    fakeMillis += RetryMs; MDNS.startOk = true;
    maintainConnection(); assert(MDNS.starts == 2 && MDNS.queries == 1);
    assert(MDNS.lastHost == "Windows-Host");
    unsigned connects = service.connects;
    fakeMillis += RetryMs; MDNS.result = 0x090A0B0C;
    maintainConnection();
    assert(MDNS.queries == 2 && service.connects == connects + 1);
    assert(service.lastAddress == MDNS.result);
    strcpy(activeConfig.host, "192.168.50.90");
    service.stop(); fakeMillis += RetryMs;
    maintainConnection(); assert(WiFi.resolutions == 3 && MDNS.queries == 2);
    WiFi.statusValue = 0; fakeMillis += RetryMs;
    maintainConnection(); assert(!service.connected() && WiFi.reconnects == 1);
    WiFi.statusValue = WL_CONNECTED;
    setup(); // Simulate startup loading the saved snapshot, not unsaved RAM.
    assert(!strcmp(activeConfig.host, "Staged.local"));
    assert(radio.frequency == 916.5f && radio.power == 10);
    assert(radio.syncWord == 0xAC && radio.controlBits == 0xBD);
    assert(!console.active);
    std::cout << "Gateway runtime tests passed\n";
}
