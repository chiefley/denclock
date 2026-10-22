#pragma once
#include <string>
#include <map>
#include <string.h>
static std::string flash;
static unsigned writes;
static bool failSave;
static std::map<std::string, std::string> otherFlash;
class Preferences {
    std::string ns;
    std::string& data(const char* key) {
        return ns == "denclock-wifi" ? otherFlash[ns + "/" + key] : flash;
    }
public:
    bool begin(const char* name, bool) { ns = name; return true; }
    void end() {}
    size_t getBytesLength(const char* key) { return data(key).size(); }
    size_t getBytes(const char* key, void* p, size_t n) {
        n = std::min(n, data(key).size()); memcpy(p, data(key).data(), n); return n;
    }
    size_t putBytes(const char* key, const void* p, size_t n) {
        ++writes;
        if (failSave) return 0;
        data(key).assign(static_cast<const char*>(p), n); return n;
    }
};
