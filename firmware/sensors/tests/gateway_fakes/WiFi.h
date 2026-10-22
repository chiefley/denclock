#pragma once
#include "Arduino.h"
#include <string.h>
#include <memory>
#include <vector>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
class IPAddress {
    uint32_t value = 0;
public:
    IPAddress() = default;
    IPAddress(unsigned a,unsigned b,unsigned c,unsigned d) : value((a<<24)|(b<<16)|(c<<8)|d) {}
    explicit IPAddress(uint32_t n) : value(n) {}
    bool fromString(const char* text) {
        unsigned a,b,c,d; char tail;
        if (sscanf(text, "%u.%u.%u.%u%c", &a,&b,&c,&d,&tail) != 4 || a>255 || b>255 || c>255 || d>255) return false;
        value = (a<<24) | (b<<16) | (c<<8) | d;
        return true;
    }
    std::string toString() const {
        char text[16];
        snprintf(text, sizeof(text), "%u.%u.%u.%u", value >> 24,
                 (value >> 16) & 255, (value >> 8) & 255, value & 255);
        return text;
    }
    operator uint32_t() const { return value; }
};
constexpr int WL_CONNECTED = 3, WIFI_STA = 1, WIFI_AP_STA = 3, WIFI_SCAN_RUNNING = -1;
enum WiFiEvent_t { ARDUINO_EVENT_WIFI_STA_CONNECTED, ARDUINO_EVENT_WIFI_STA_GOT_IP, ARDUINO_EVENT_WIFI_STA_DISCONNECTED };
struct WiFiEventInfo_t { struct { int reason; } wifi_sta_disconnected; };
struct FakeWiFi {
    int statusValue = 0, modeValue = WIFI_STA;
    bool ap = false, apOk = true, persistentValue = true, autoReconnect = true;
    unsigned begins = 0;
    std::string stationSsid, stationPassword, apSsid, apPassword;
    uint32_t localAddress = 0xC0A8014D;
    int scanValue = 0;
    std::vector<std::string> scanNames;
    void persistent(bool value) { persistentValue=value; }
    void macAddress(uint8_t* mac) { for (int i=0;i<6;++i) mac[i]=uint8_t(i+1); }
    bool softAPConfig(IPAddress ip, IPAddress, IPAddress) { assert(uint32_t(ip)==0xC0A80401); return apOk; }
    bool softAP(const char* name, const char* pass, int, bool, int) {
        ap=apOk; apSsid=name; apPassword=pass; return apOk;
    }
    void softAPdisconnect(bool) { ap=false; }
    void disconnect(bool, bool) { statusValue=0; }
    int scanComplete() { return scanValue; }
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
    IPAddress localIP() { return IPAddress(localAddress); }
    void onEvent(void (*)(WiFiEvent_t, WiFiEventInfo_t)) {}
    void mode(int value) { modeValue=value; }
    int scanNetworks(bool = false, bool = false, bool = false, unsigned = 0) { return WIFI_SCAN_RUNNING; }
    FakeString SSID(int i) { return FakeString(scanNames.at(size_t(i))); }
    int channel(int) { return 1; }
    int encryptionType(int) { return 1; }
    FakeString BSSIDstr(int) { return FakeString{}; }
    void scanDelete() {}
    void setAutoReconnect(bool value) { autoReconnect=value; }
    void begin(const char* name, const char* pass) { ++begins; stationSsid=name; stationPassword=pass; statusValue=0; }
} static WiFi;
static unsigned browserReads=0;
struct FakeSocket {
    int value;
    explicit FakeSocket(int n) : value(n) {}
    ~FakeSocket() { if(value>=0) ::close(value); }
};
class WiFiClient {
    std::shared_ptr<FakeSocket> socket;
public:
    WiFiClient() = default;
    explicit WiFiClient(int n) : socket(std::make_shared<FakeSocket>(n)) { connectedValue=true; }
    int fd() const { return socket ? socket->value : -1; }
    explicit operator bool() { return connected(); }
    int available() { int n=0; if (fd()>=0) ioctl(fd(),FIONREAD,&n); return n; }
    int read() { char c; if(recv(fd(),&c,1,MSG_DONTWAIT)!=1) return -1; ++browserReads; return static_cast<unsigned char>(c); }
    bool connectedValue = false, connectOk = true;
    unsigned connects = 0;
    uint32_t lastAddress = 0;
    std::string frames;
    bool connected() { return connectedValue; }
    void stop() { connectedValue = false; if(socket && socket->value>=0) { ::close(socket->value); socket->value=-1; } }
    bool connect(IPAddress ip, unsigned port, unsigned timeout) {
        assert(port == 5210 && timeout == 1000);
        ++connects; lastAddress = uint32_t(ip); connectedValue = connectOk;
        return connectedValue;
    }
    size_t write(const uint8_t* bytes, size_t n) { frames.append(reinterpret_cast<const char*>(bytes), n); return n; }
    void setNoDelay(bool) {}
    void setTimeout(unsigned) {}
};

static int pendingBrowser = -1;
static bool httpListening = false;
class WiFiServer {
public:
    WiFiServer(IPAddress, unsigned port, unsigned clients) { assert(port==80 && clients==1); }
    explicit operator bool() { return httpListening; }
    void begin() { httpListening=true; }
    void end() { httpListening=false; }
    WiFiClient available() {
        if (pendingBrowser<0) return WiFiClient{};
        int fd=pendingBrowser; pendingBrowser=-1; return WiFiClient(fd);
    }
};
