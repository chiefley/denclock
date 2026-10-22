#pragma once
#include <stdint.h>

struct RadioLibSync {
    uint8_t syncWord;
    uint8_t controlBits;
};
// Pack high nibbles into syncWord and low nibbles into controlBits.
// Register bytes D3 C7 require RadioLib arguments DC, 37.
inline RadioLibSync radioLibSync(uint16_t registers) {
    return {uint8_t(((registers >> 8) & 0xF0) | ((registers >> 4) & 0x0F)),
            uint8_t(((registers >> 4) & 0xF0) | (registers & 0x0F))};
}
