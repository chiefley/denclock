#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

namespace GatewayProtocol {
constexpr size_t MaxLoRaLength = 255;
constexpr size_t MaxFrameLength = 3 + 2 + MaxLoRaLength;
inline void encodeHello(uint8_t (&frame)[4], uint8_t gatewayId) {
    frame[0] = 1; frame[1] = 0; frame[2] = 1; frame[3] = gatewayId;
}
// Round to the nearest wire unit. Reject values outside int8 instead of wrapping.
inline bool encodeSignal(float value, float scale, uint8_t& encoded) {
    if (!isfinite(value)) return false;
    const float rounded = roundf(value * scale);
    if (rounded < -128 || rounded > 127) return false;
    encoded = static_cast<uint8_t>(static_cast<int8_t>(rounded));
    return true;
}
inline size_t encodeRx(uint8_t (&frame)[MaxFrameLength], const uint8_t* packet,
                       size_t length, float rssi, float snr) {
    if (length > MaxLoRaLength) return 0;
    if (!encodeSignal(rssi, 1, frame[3]) || !encodeSignal(snr, 4, frame[4])) return 0;
    const size_t payloadLength = length + 2;
    frame[0] = 2;
    frame[1] = payloadLength >> 8;
    frame[2] = payloadLength & 0xff;
    memcpy(frame + 5, packet, length);
    return length + 5;
}
} // namespace GatewayProtocol
