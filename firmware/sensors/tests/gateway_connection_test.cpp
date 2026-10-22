#include <assert.h>
#include <string.h>
#include <iostream>
#include "../lora-gateway/include/gateway_connection.h"
struct FakeResolver {
    uint32_t address = 0; // Zero means the lookup fails.
    unsigned lookups = 0;
    bool hostByName(const char* host, IPAddress& result) {
        ++lookups;
        assert(!strcmp(host, "denclock-d3c7.local"));
        if (!address) return false;
        result.value = address;
        return true;
    }
};
struct FakeClient {
    bool accept = true;
    unsigned attempts = 0;
    uint32_t address = 0;
    uint16_t port = 0;
    bool connect(IPAddress target, uint16_t targetPort, int32_t timeoutMs) {
        assert(timeoutMs == 1000); // Existing connect timeout.
        ++attempts;
        address = target.value;
        port = targetPort;
        return accept;
    }
};
int main() {
    FakeResolver resolver;
    FakeClient client;
    IPAddress address;
    // Unresolved names never attempt TCP and leave the next attempt free to retry.
    assert(connectService(resolver, client, "denclock-d3c7.local", 5210, address) == ServiceConnect::Unresolved);
    assert(connectService(resolver, client, "denclock-d3c7.local", 5210, address) == ServiceConnect::Unresolved);
    assert(resolver.lookups == 2 && client.attempts == 0);
    // Recovery requires no restart.
    resolver.address = 0x0a000002;
    assert(connectService(resolver, client, "denclock-d3c7.local", 5210, address) == ServiceConnect::Connected);
    assert(client.attempts == 1 && client.address == 0x0a000002 && client.port == 5210);
    // Every attempt resolves again: a changed address is used, never a cached one.
    resolver.address = 0x0a000003;
    client.accept = false;
    assert(connectService(resolver, client, "denclock-d3c7.local", 5210, address) == ServiceConnect::Failed);
    assert(resolver.lookups == 4 && client.address == 0x0a000003 && address.value == 0x0a000003);
    client.accept = true;
    resolver.address = 0;
    assert(connectService(resolver, client, "denclock-d3c7.local", 5210, address) == ServiceConnect::Unresolved);
    assert(client.attempts == 2);
    std::cout << "Gateway connection tests passed\n";
}
