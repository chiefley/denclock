#include "../lora-gateway/include/gateway_protocol.h"
#include <assert.h>
#include <stdio.h>
#include <limits>
using namespace GatewayProtocol;
int main() {
    uint8_t hello[4];
    encodeHello(hello, 1);
    const uint8_t expectedHello[] = {1,0,1,1};
    assert(memcmp(hello, expectedHello, sizeof(hello)) == 0);
    const uint8_t sensor[] = {1,12,0x0f,0x1f,1,2,0x08,0x5f,2,2,0x10,0xe1,3,2,0x0f,0x48};
    const uint8_t expected[] = {2,0,18,0xb8,38,1,12,0x0f,0x1f,1,2,0x08,0x5f,2,2,0x10,0xe1,3,2,0x0f,0x48};
    uint8_t frame[MaxFrameLength];
    assert(encodeRx(frame, sensor, sizeof(sensor), -72, 9.5) == sizeof(expected));
    assert(memcmp(frame, expected, sizeof(expected)) == 0);
    uint8_t opaque[255];
    for (size_t i = 0; i < sizeof(opaque); ++i) opaque[i] = uint8_t(i);
    assert(encodeRx(frame, opaque, sizeof(opaque), -128, -3) == 260);
    assert(frame[0] == 2 && frame[1] == 1 && frame[2] == 1);
    assert(frame[3] == 128 && frame[4] == 244);
    assert(memcmp(frame + 5, opaque, sizeof(opaque)) == 0); // Includes class zero: transport is opaque.
    assert(encodeRx(frame, opaque, 256, -72, 0) == 0);
    assert(encodeRx(frame, opaque, 1, -129, 0) == 0);
    assert(encodeRx(frame, opaque, 1, -72, 32) == 0);
    assert(encodeRx(frame, opaque, 1, std::numeric_limits<float>::infinity(), 0) == 0);
    assert(encodeRx(frame, opaque, 1, -72, std::numeric_limits<float>::quiet_NaN()) == 0);
    assert(encodeRx(frame, opaque, 1, -72.5, -0.25) == 6);
    assert(frame[3] == 183 && frame[4] == 255);
    puts("Gateway Protocol encoder tests passed");
}
