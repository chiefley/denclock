#pragma once
#include <assert.h>
#include <stdint.h>
#include <string>
using esp_err_t = int;
constexpr esp_err_t ESP_OK = 0;
struct esp_ip4_addr_t { uint32_t addr; };
static unsigned mdnsInitializations = 0, mdnsQueries = 0;
static esp_err_t mdnsInitResult = -1, mdnsQueryResult = -1;
static uint32_t mdnsAddress = 0x05060708;
static std::string mdnsLastHost;
inline esp_err_t mdns_init() { ++mdnsInitializations; return mdnsInitResult; }
inline esp_err_t mdns_query_a(const char* host, uint32_t timeout, esp_ip4_addr_t* result) {
    assert(timeout == 2000);
    ++mdnsQueries; mdnsLastHost = host; result->addr = mdnsAddress;
    return mdnsQueryResult;
}
// Deliberately no hostname or service-registration API: query-only contract.
