#pragma once
#include "Arduino.h"
#include <string.h>
class IPAddress {
    uint32_t value = 0;
public:
    IPAddress() = default;
    explicit IPAddress(uint32_t n) : value(n) {}
    bool fromString(const char* text) {
        unsigned a,b,c,d; char tail;
        if (sscanf(text, "%u.%u.%u.%u%c", &a,&b,&c,&d,&tail) != 4 || a>255 || b>255 || c>255 || d>255) return false;
        value = (a<<24) | (b<<16) | (c<<8) | d;
        return true;
    }
    operator uint32_t() const { return value; }
};
constexpr int WL_CONNECTED = 3, WIFI_STA = 1;
enum WiFiEvent_t { ARDUINO_EVENT_WIFI_STA_CONNECTED, ARDUINO_EVENT_WIFI_STA_GOT_IP, ARDUINO_EVENT_WIFI_STA_DISCONNECTED };
struct WiFiEventInfo_t { struct { int reason; } wifi_sta_disconnected; };
struct FakeWiFi {
    int statusValue = WL_CONNECTED;
    unsigned resolutions = 0, reconnects = 0;
    bool resolveOk = false;
    std::string lastHost;
    uint32_t resolvedAddress = 0x01020304;
    int status() { return statusValue; }
    int RSSI(int = 0) { return -50; }
    void reconnect() { ++reconnects; }
    int hostByName(const char* host, IPAddress& ip) {
        ++resolutions; lastHost = host;
        ip = IPAddress(resolveOk ? resolvedAddress : 0);
        return resolveOk ? 1 : 0;
    }
    IPAddress localIP() { return IPAddress(1); }
    void onEvent(void (*)(WiFiEvent_t, WiFiEventInfo_t)) {}
    void mode(int) {}
    int scanNetworks() { return 0; }
    FakeString SSID(int) { return {}; }
    int channel(int) { return 1; }
    int encryptionType(int) { return 1; }
    FakeString BSSIDstr(int) { return {}; }
    void scanDelete() {}
    void setAutoReconnect(bool) {}
    void begin(const char*, const char*) {}
} static WiFi;
class WiFiClient {
public:
    bool connectedValue = false, connectOk = true;
    unsigned connects = 0;
    uint32_t lastAddress = 0;
    std::string frames;
    bool connected() { return connectedValue; }
    void stop() { connectedValue = false; }
    bool connect(IPAddress ip, unsigned port, unsigned timeout) {
        assert(port == 5210 && timeout == 1000);
        ++connects; lastAddress = uint32_t(ip); connectedValue = connectOk;
        return connectedValue;
    }
    size_t write(const uint8_t* bytes, size_t n) { frames.append(reinterpret_cast<const char*>(bytes), n); return n; }
    void setNoDelay(bool) {}
    void setTimeout(unsigned) {}
};
