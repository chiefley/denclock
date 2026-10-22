#include <assert.h>
#include <iostream>
#include "../common/radio_sync.h"
int main() {
    const RadioLibSync defaults = radioLibSync(0xD3C7);
    assert(defaults.syncWord == 0xDC && defaults.controlBits == 0x37);
    // Reconstruct the SX1262 bytes for every possible register value.
    for (uint32_t value = 0; value <= 0xFFFF; ++value) {
        const RadioLibSync p = radioLibSync(uint16_t(value));
        uint8_t first = (p.syncWord & 0xF0) | (p.controlBits >> 4);
        uint8_t second = (p.syncWord << 4) | (p.controlBits & 0x0F);
        assert(((uint32_t(first) << 8) | second) == value);
    }
    std::cout << "Radio sync conversion tests passed\n";
}
