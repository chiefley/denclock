#include "../lora-gateway/include/gateway_diagnostics.h"
#include "../lora-gateway/include/gateway_protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    GatewayDiagnostics d;
    assert(d.accepted == 0 && d.rejected == 0 && d.node == 0);
    assert(!d.errorVisible && !d.lastError && !d.hasBattery && d.dirty);
    uint8_t packet[SensorReport::PacketSize];
    SensorReport::encode(packet, 3, 1, 2345, 5025, 3912);
    assert(d.observe(packet, sizeof(packet), -87, 8, 100));
    assert(d.accepted == 1 && d.rejected == 0 && d.node == 3);
    assert(d.hasBattery && d.battery == 3912 && d.rssi == -87 && d.snr == 8);

    // Decoder rejection must neither publish partial data nor alter raw transport.
    packet[13] = 3; // Battery length exceeds the remaining payload.
    uint8_t original[sizeof(packet)];
    memcpy(original, packet, sizeof(packet));
    assert(!d.observe(packet, sizeof(packet), -90, 1, 200));
    assert(d.accepted == 1 && d.rejected == 1 && d.node == 3 && d.battery == 3912);
    assert(!strcmp(d.lastError, "truncated TLV value") && d.lastRadioError == 0);
    uint8_t frame[GatewayProtocol::MaxFrameLength];
    assert(GatewayProtocol::encodeRx(frame, packet, sizeof(packet), -90, 1) == 21);
    assert(!memcmp(frame + 5, original, sizeof(original)));

    // A valid packet during the alert updates the cached node, not the screen.
    d.dirty = false;
    SensorReport::encode(packet, 239, 2, -500, 10000, 4200);
    assert(d.observe(packet, sizeof(packet), -100, -4, 300));
    assert(d.accepted == 2 && d.node == 239 && d.battery == 4200);
    assert(d.errorVisible && !d.dirty && d.lastError);
    d.tick(3199);
    assert(d.errorVisible && !d.dirty);
    d.tick(3200);
    assert(!d.errorVisible && d.dirty && d.node == 239);

    // An absent battery is legal and must not inherit the previous node's value.
    const uint8_t headerOnly[] = {1, 2, 0, 0};
    assert(d.observe(headerOnly, sizeof(headerOnly), -80, 7, 4000));
    assert(d.accepted == 3 && d.node == 2 && !d.hasBattery && d.battery == 0);

    packet[0] = 2;
    assert(!d.observe(packet, sizeof(packet), -90, 0, 4100));
    assert(!strcmp(d.lastError, "unsupported message class"));
    packet[0] = 1; packet[1] = 0;
    assert(!d.observe(packet, sizeof(packet), -90, 0, 4200));
    assert(!strcmp(d.lastError, "invalid source node"));
    assert(d.rejected == 3 && d.accepted == 3);
    d.tick(7100); // Second rejection extended the interval.
    assert(d.errorVisible);
    d.tick(7200);
    assert(!d.errorVisible);

    // Observed radio failures retain their exact code, including before first RX.
    GatewayDiagnostics boot;
    boot.reject("radio packet failure", -7, UINT32_MAX - 1000);
    assert(boot.rejected == 1 && boot.accepted == 0 && boot.node == 0);
    assert(boot.lastRadioError == -7 && !strcmp(boot.lastError, "radio packet failure"));
    boot.dirty = false;
    boot.tick(1998);
    assert(boot.errorVisible && !boot.dirty);
    boot.tick(1999);
    assert(!boot.errorVisible && boot.dirty);
    assert(boot.observe(headerOnly, sizeof(headerOnly), -80, 7, 2000));
    assert(boot.lastRadioError == -7 && boot.lastError); // Success retains reason.
    puts("gateway diagnostics tests passed");
}
