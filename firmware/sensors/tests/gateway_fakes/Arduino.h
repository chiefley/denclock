#pragma once
#include <stdint.h>
#include <string>
#include <stdio.h>
#include <stddef.h>
#define IRAM_ATTR
constexpr int Vext = 36, OUTPUT = 1, LOW = 0, HIGH = 1;
static uint32_t fakeMillis;
inline uint32_t millis() { return fakeMillis; }
inline void delay(uint32_t ms) { fakeMillis += ms; }
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
struct FakeString {
    const char* c_str() const { return "fake"; }
};
struct FakeSerial {
    std::string input, output;
    void begin(unsigned) {}
    bool available() { return !input.empty(); }
    char read() { char c = input[0]; input.erase(0, 1); return c; }
    template<typename T> void print(const T&) {}
    template<typename T> void println(const T&) {}
    void println(const char* text) { output += text; output += '\n'; }
    void printf(const char* text) { output += text; }
    template<typename... Args> void printf(const char* format, Args... args) {
        char buffer[2048]; snprintf(buffer, sizeof(buffer), format, args...); output += buffer;
    }
    void flush() {}
} static Serial;
struct FakeEsp { bool restarted = false; void restart() { restarted = true; } } static ESP;
