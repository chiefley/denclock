#pragma once
#include <stdint.h>
#include <string.h>
constexpr int RADIOLIB_ERR_NONE = 0, RADIOLIB_SX126X_SYNC_WORD_PRIVATE = 0x12;
struct Module { Module(int,int,int,int) {} };
struct SX1262 {
    float tcxoVoltage = 0, frequency = 0;
    uint8_t syncWord = 0, controlBits = 0;
    int8_t power = 0;
    unsigned receiveStarts = 0;
    uint8_t packet[16]{};
    SX1262(Module*) {}
    int16_t begin(float f,float bandwidth,int sf,int cr,uint8_t,int8_t p,int preamble,float tcxo) {
        assert(bandwidth == 125 && sf == 9 && cr == 7 && preamble == 8 && tcxo == 1.8f);
        frequency = f; power = p; return 0;
    }
    int16_t setSyncWord(uint8_t a,uint8_t b) { syncWord=a; controlBits=b; return 0; }
    int16_t setCRC(int) { return 0; }
    int16_t startReceive() { ++receiveStarts; return 0; }
    void setPacketReceivedAction(void (*)()) {}
    void standby() {}
    size_t getPacketLength() { return sizeof(packet); }
    int16_t readData(uint8_t* out, size_t n) { memcpy(out, packet, n); return 0; }
    float getRSSI() { return -60; }
    float getSNR() { return 5; }
};
