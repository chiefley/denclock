#pragma once
#include <Arduino.h>

enum class ServiceConnect { Unresolved, Failed, Connected };
// Resolve on every attempt so a failed lookup or a changed address never sticks.
// lwIP resolves IPv4 literals directly, DNS names via DHCP-provided servers, and
// .local names via multicast DNS queries.
template <typename Resolver, typename Client>
ServiceConnect connectService(Resolver& resolver, Client& client, const char* host,
                              uint16_t port, IPAddress& address) {
    if (!resolver.hostByName(host, address)) return ServiceConnect::Unresolved;
    if (!client.connect(address, port, 1000)) return ServiceConnect::Failed;
    return ServiceConnect::Connected;
}
