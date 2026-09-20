#include "../common/sensor_report.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <vector>
using namespace SensorReport;
int main() {
    const uint8_t expected[] = {1,1,0x12,0x34,1,2,0xfb,0x1e,2,2,0x10,0xe1,3,2,0x0f,0x48};
    uint8_t packet[PacketSize];
    encode(packet, 1, 0x1234, -1250, 4321, 3912);
    assert(memcmp(packet, expected, sizeof(packet)) == 0);
    Report r;
    assert(!decode(expected, sizeof(expected), r));
    assert(r.node == 1 && r.sequence == 0x1234 && r.temperature == -1250);
    assert(r.humidity == 4321 && r.battery == 3912);
    assert(nextSequence(0) == 1 && nextSequence(65534) == 65535 && nextSequence(65535) == 1);
    for (size_t n = 0; n < sizeof(packet); ++n) {
        // Complete prefixes are legal: protocol allows zero or more TLVs.
        bool valid = n == 4 || n == 8 || n == 12;
        assert((decode(packet, n, r) == nullptr) == valid);
    }
    for (int value : {0, 2, 255}) {
        packet[0] = value; assert(decode(packet, sizeof(packet), r));
    }
    memcpy(packet, expected, sizeof(packet));
    for (int value : {0, 240, 254, 255}) {
        packet[1] = value; assert(decode(packet, sizeof(packet), r));
    }
    memcpy(packet, expected, sizeof(packet));
    packet[4] = 0; assert(decode(packet, sizeof(packet), r));
    memcpy(packet, expected, sizeof(packet));
    for (int length : {0, 1, 3, 255}) {
        packet[5] = length; assert(decode(packet, sizeof(packet), r));
    }
    memcpy(packet, expected, sizeof(packet));
    packet[10] = 0x27; packet[11] = 0x11;
    assert(decode(packet, sizeof(packet), r));
    std::vector<uint8_t> extended(expected, expected + sizeof(expected));
    extended.insert(extended.begin() + 4, {42, 3, 0, 0xff, 0x80});
    assert(!decode(extended.data(), extended.size(), r));
    assert(r.temperature == -1250 && r.battery == 3912);
    extended.push_back(42); assert(decode(extended.data(), extended.size(), r));
    extended = std::vector<uint8_t>(expected, expected + sizeof(expected));
    extended.insert(extended.end(), expected + 4, expected + 8);
    r.sequence = 99;
    assert(decode(extended.data(), extended.size(), r));
    assert(r.sequence == 99); // Failed validation never publishes partial data.
    const uint8_t reordered[] = {1,1,0,0,3,2,0,0,2,2,0x27,0x10,1,2,0x80,0};
    assert(!decode(reordered, sizeof(reordered), r));
    assert(r.temperature == -32768 && r.humidity == 10000 && r.battery == 0);
    encode(packet, 1, 65535, 32767, 0, 65535);
    assert(!decode(packet, sizeof(packet), r));
    assert(r.temperature == 32767 && r.sequence == 65535 && r.battery == 65535);
    const uint8_t header[] = {1,1,0,0};
    assert(!decode(header, sizeof(header), r));
    assert(!r.hasTemperature && !r.hasHumidity && !r.hasBattery);
    // Exercise every TLV type/length combination against a short buffer.
    for (int type = 0; type < 256; ++type)
        for (int length = 0; length < 256; ++length) {
            uint8_t probe[] = {1,1,0,0,uint8_t(type),uint8_t(length),0,0};
            decode(probe, sizeof(probe), r);
        }
    puts("sensor report tests passed");
}
