#pragma once
#include <RadioLib.h>
#include "radio_sync.h"

// Heltec WiFi LoRa 32 V3: NSS, DIO1, reset, busy.
constexpr int RadioNss = 8, RadioDio1 = 14, RadioReset = 12, RadioBusy = 13;
// Defaults preserve the gateway radio configuration; sensors pass saved settings.
inline int16_t initializeRadio(SX1262& radio, int8_t power = 10,
                               int32_t frequencyKhz = 915000, uint16_t sync = 0xD3C7) {
    radio.tcxoVoltage = 1.8;
    // Eight symbols is RadioLib's default; explicitly retain the board's 1.8 V TCXO.
    int16_t state = radio.begin(frequencyKhz / 1000.0f, 125.0, 9, 7,
                               RADIOLIB_SX126X_SYNC_WORD_PRIVATE, power, 8, 1.8);
    if (state != RADIOLIB_ERR_NONE) return state;
    const RadioLibSync parameters = radioLibSync(sync);
    state = radio.setSyncWord(parameters.syncWord, parameters.controlBits);
    if (state != RADIOLIB_ERR_NONE) return state;
    return radio.setCRC(2);
}
