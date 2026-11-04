#pragma once
#include <Arduino.h>
#include <atomic>

// Only unsolicited gateway diagnostics use this gate. Console output stays on
// Serial. Bind once before Wi-Fi callbacks are registered; no messages are kept.
namespace GatewaySerial {
inline const std::atomic<bool>*& consoleState() {
    static const std::atomic<bool>* state = nullptr;
    return state;
}
inline void bind(const std::atomic<bool>& active) { consoleState() = &active; }
inline bool enabled() { return !consoleState() || !consoleState()->load(); }
template<typename T> inline void print(const T& value) {
    if (enabled()) Serial.print(value);
}
template<typename T> inline void println(const T& value) {
    if (enabled()) Serial.println(value);
}
template<typename... Args> inline void printf(const char* format, Args... args) {
    if (enabled()) Serial.printf(format, args...);
}
} // namespace GatewaySerial
