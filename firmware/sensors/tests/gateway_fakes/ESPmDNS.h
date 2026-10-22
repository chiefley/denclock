#pragma once
#include "WiFi.h"
struct FakeMdns {
    unsigned queries = 0, starts = 0;
    bool startOk = true;
    std::string lastHost;
    uint32_t result = 0;
    bool begin(const char*) { ++starts; return startOk; }
    IPAddress queryHost(const char* name, uint32_t timeout) {
        assert(timeout == 1000);
        ++queries; lastHost = name; return IPAddress(result);
    }
} static MDNS;
