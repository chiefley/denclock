#pragma once
#include <stdint.h>
#include <stddef.h>
inline void esp_fill_random(void* data, size_t n) {
    static uint8_t next=0;
    for(size_t i=0;i<n;++i) static_cast<uint8_t*>(data)[i]=next++;
}
