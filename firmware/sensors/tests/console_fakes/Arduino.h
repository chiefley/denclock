#pragma once
#include <stdint.h>
#include <string>
#include <stdio.h>
static uint32_t fakeMillis;
inline uint32_t millis() { return fakeMillis; }
struct FakeSerial {
    std::string input, output;
    bool available() { return !input.empty(); }
    char read() { char c = input[0]; input.erase(0, 1); return c; }
    void println(const char* s) { output += s; output += '\n'; }
    void printf(const char* text) { output += text; }
    template<typename... Args> void printf(const char* format, Args... args) {
        char buffer[1024]; snprintf(buffer, sizeof(buffer), format, args...); output += buffer;
    }
    void flush() {}
} static Serial;
struct IPAddress { uint32_t value = 0; };
struct FakeEsp { bool restarted = false; void restart() { restarted = true; } } static ESP;
