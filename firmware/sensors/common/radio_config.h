#pragma once
#include <RadioLib.h>

// Heltec WiFi LoRa 32 V3: NSS, DIO1, reset, busy.
constexpr int RadioNss = 8, RadioDio1 = 14, RadioReset = 12, RadioBusy = 13;
inline int16_t initializeRadio(SX1262& radio) {
    radio.tcxoVoltage = 1.8;
    // Eight symbols is RadioLib's default; explicitly retain the board's 1.8 V TCXO.
    int16_t state = radio.begin(915.0, 125.0, 9, 7,
                               RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 10, 8, 1.8);
    if (state != RADIOLIB_ERR_NONE) return state;
    // RadioLib expands the high nibbles from syncWord and low nibbles
    // from controlBits: 0xDC, 0x37 -> register bytes 0xD3, 0xC7.
    state = radio.setSyncWord(0xDC, 0x37);
    if (state != RADIOLIB_ERR_NONE) return state;
    return radio.setCRC(2);
}
