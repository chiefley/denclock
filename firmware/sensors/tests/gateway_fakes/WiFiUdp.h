#pragma once
#include "WiFi.h"
static bool dnsRunning=false;
static unsigned dnsPolls=0;
struct WiFiUDP {
    int begin(IPAddress, unsigned port) { assert(port==53); dnsRunning=true; return 1; }
    void stop() { dnsRunning=false; }
    int parsePacket() { ++dnsPolls; return 0; }
    void flush() {}
    int read(uint8_t*,size_t) { return 0; }
    IPAddress remoteIP() { return IPAddress(1); }
    unsigned remotePort() { return 1234; }
    void beginPacket(IPAddress,unsigned) {}
    void write(uint8_t*,size_t) {}
    void endPacket() {}
};
