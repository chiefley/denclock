#pragma once
#include <stddef.h>
#include <stdint.h>

namespace SensorReport {
constexpr size_t PacketSize = 16;
inline uint16_t nextSequence(uint16_t sequence) {
    return sequence == 65535 ? 1 : sequence + 1;
}
inline void put16(uint8_t* out, uint16_t value) {
    out[0] = value >> 8;
    out[1] = value & 0xff;
}
inline uint16_t get16(const uint8_t* in) {
    return (uint16_t(in[0]) << 8) | in[1];
}
inline void encode(uint8_t (&out)[PacketSize], uint8_t node, uint16_t sequence,
                   int16_t temperature, uint16_t humidity, uint16_t battery) {
    out[0] = 1; out[1] = node;
    put16(out + 2, sequence);
    out[4] = 1; out[5] = 2; put16(out + 6, uint16_t(temperature));
    out[8] = 2; out[9] = 2; put16(out + 10, humidity);
    out[12] = 3; out[13] = 2; put16(out + 14, battery);
}
struct Report {
    uint8_t node = 0;
    uint16_t sequence = 0;
    int16_t temperature = 0;
    uint16_t humidity = 0, battery = 0;
    bool hasTemperature = false, hasHumidity = false, hasBattery = false;
};
// Validate the entire packet before making any measurements available.
// Missing measurements and unknown nonzero TLVs are legal in the protocol.
inline const char* decode(const uint8_t* data, size_t size, Report& output) {
    if (size < 4 || size > 255) return "invalid packet length";
    if (data[0] != 1) return "unsupported message class";
    if (data[1] == 0 || data[1] > 0xef) return "invalid source node";
    Report r;
    r.node = data[1]; r.sequence = get16(data + 2);
    for (size_t pos = 4; pos < size;) {
        if (size - pos < 2) return "truncated TLV header";
        uint8_t type = data[pos++], length = data[pos++];
        if (type == 0) return "reserved TLV type";
        if (length > size - pos) return "truncated TLV value";
        if (type <= 3) {
            if (length != 2) return "invalid measurement length";
            uint16_t value = get16(data + pos);
            if (type == 1) {
                if (r.hasTemperature) return "duplicate temperature";
                r.hasTemperature = true;
                r.temperature = value < 0x8000 ? int32_t(value) : int32_t(value) - 65536;
            } else if (type == 2) {
                if (r.hasHumidity) return "duplicate humidity";
                if (value > 10000) return "humidity out of range";
                r.hasHumidity = true; r.humidity = value;
            } else {
                if (r.hasBattery) return "duplicate battery";
                r.hasBattery = true; r.battery = value;
            }
        }
        pos += length;
    }
    output = r;
    return nullptr;
}
} // namespace SensorReport
