#pragma once
#include <string>
#include <string.h>
static std::string flash;
static unsigned writes;
static bool failSave;
class Preferences {
public:
    bool begin(const char*, bool) { return true; }
    void end() {}
    size_t getBytesLength(const char*) { return flash.size(); }
    size_t getBytes(const char*, void* p, size_t n) { memcpy(p, flash.data(), n); return n; }
    size_t putBytes(const char*, const void* p, size_t n) {
        ++writes;
        if (failSave) return 0;
        flash.assign(static_cast<const char*>(p), n); return n;
    }
};
