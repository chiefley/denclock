#pragma once
#include <stdint.h>

// Optional service endpoint overrides; household Wi-Fi is provisioned at runtime.
#if __has_include("config.local.h")
#include "config.local.h"
#endif
#ifndef LORA_SERVICE_HOST
#define LORA_SERVICE_HOST "192.168.50.90"
#endif
#ifndef LORA_SERVICE_PORT
#define LORA_SERVICE_PORT 5210
#endif
constexpr uint8_t GatewayId = 1;
static_assert(LORA_SERVICE_PORT > 0 && LORA_SERVICE_PORT <= 65535, "Invalid TCP port");
