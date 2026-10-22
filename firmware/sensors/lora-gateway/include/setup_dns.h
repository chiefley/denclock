#pragma once
#include <WiFiUdp.h>
#include <stdint.h>
#include <string.h>
namespace GatewayWiFi {
// Captive DNS: a single bounded question, no compressed query names. AAAA
// receives an empty answer; A/ANY receives only the local setup IPv4 address.
inline size_t dnsReply(uint8_t* data, size_t n, size_t capacity) {
    if (n<17 || n>512 || data[2]&0xF8 || data[4]!=0 || data[5]!=1 ||
        data[6] || data[7] || data[8] || data[9]) return 0;
    size_t p=12;
    while (p<n && data[p]) {
        const size_t label=data[p++];
        if (label>63 || label>n-p || p+label-12>254) return 0;
        p+=label;
    }
    if (p>=n || p+5>n) return 0;
    ++p;
    const unsigned type=unsigned(data[p])*256+data[p+1];
    if (data[p+2]!=0 || data[p+3]!=1) return 0;
    p+=4;
    const bool answer=type==1 || type==255;
    if (p+(answer?16:0)>capacity) return 0;
    data[2]=uint8_t(0x80|(data[2]&1)); data[3]=0;
    data[6]=0; data[7]=answer?1:0;
    data[8]=data[9]=data[10]=data[11]=0;
    if (answer) {
        const uint8_t record[]={0xC0,12,0,1,0,1,0,0,0,0,0,4,192,168,4,1};
        memcpy(data+p,record,sizeof(record)); p+=sizeof(record);
    }
    return p;
}
struct SetupDns {
    WiFiUDP udp;
    bool start(IPAddress ip) { return udp.begin(ip,53)==1; }
    void stop() { udp.stop(); }
    void tick() {
        int n=udp.parsePacket();
        if (n<=0) return;
        if (n>512) { udp.flush(); return; }
        uint8_t data[528];
        const int count=udp.read(data,size_t(n));
        if (count!=n) return;
        const size_t length=dnsReply(data,size_t(n),sizeof(data));
        if (!length) return;
        udp.beginPacket(udp.remoteIP(),udp.remotePort());
        udp.write(data,length); udp.endPacket();
    }
};
} // namespace GatewayWiFi
