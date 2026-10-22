#include <assert.h>
#include <iostream>
#include <random>
#include "../lora-gateway/include/wifi_provisioning.h"
#include "../lora-gateway/include/gateway_console.h"
using namespace GatewayWiFi;

std::string exchange(Provisioning& p,const std::string& request) {
    int pair[2]; assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0);
    assert(pendingBrowser<0); pendingBrowser=pair[0];
    const ssize_t wrote=::send(pair[1],request.data(),request.size(),0);
    if (wrote!=ssize_t(request.size())) std::cerr<<"send failed "<<errno<<" bytes "<<wrote<<" path "<<request.substr(0,32)<<"\n";
    assert(wrote==ssize_t(request.size()));
    std::string result;
    for (unsigned i=0;i<200;++i) {
        p.tick();
        char bytes[2048]; int n=recv(pair[1],bytes,sizeof(bytes),MSG_DONTWAIT);
        if(n>0) result.append(bytes,size_t(n));
        if(n==0) break;
        ++fakeMillis;
    }
    ::close(pair[1]);
    return result;
}
std::string post(Provisioning& p,const char* path,const std::string& fields) {
    const std::string body="token="+std::string(p.token)+fields;
    return exchange(p,std::string("POST ")+path+" HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: "+std::to_string(body.size())+"\r\n\r\n"+body);
}
Request parse(const std::string& data) { Request r; for(char c:data) r.feed(c); return r; }
void parsers() {
    assert(parse("GET / HTTP/1.1\r\nHost: setup\r\n\r\n").done);
    assert(parse("GET / HTTP/1.1\r\n"+std::string(1025,'x')).bad);
    assert(parse("POST /connect HTTP/1.1\r\nContent-Length: 513\r\n\r\n").bad);
    assert(parse("POST /connect HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 0\r\nContent-Length: 0\r\n\r\n").bad);
    assert(parse("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n").bad);
    assert(parse("GET / HTTP/1.1\r\nContent-Length: 1\r\n\r\n").bad);
    assert(parse("PUT / HTTP/1.1\r\n\r\n").bad);
    assert(parse("GET / HTTP/2\r\n\r\n").bad);
    std::string out;
    assert(decode("A+%26+%C3%A9",out,32) && out=="A & \xC3\xA9");
    assert(!decode("%00",out,32) && !decode("%X1",out,32) && !decode("%",out,32));
    assert(!decode(std::string(33,'a'),out,32));
    std::string values[5];
    assert(!fields("token=a&token=b",values));
    assert(!fields("unknown=a",values));
    assert(escape("<script>&\"'")=="&lt;script&gt;&amp;&quot;&#39;");
    uint8_t dns[528]={0,1,1,0,0,1,0,0,0,0,0,0,1,'a',0,0,1,0,1};
    uint8_t original[528]; memcpy(original,dns,sizeof(dns));
    assert(dnsReply(dns,19,sizeof(dns))==35 && dns[7]==1 && dns[31]==192);
    memcpy(dns,original,sizeof(dns)); dns[16]=28;
    assert(dnsReply(dns,19,sizeof(dns))==19 && dns[7]==0);
    memcpy(dns,original,sizeof(dns)); dns[12]=0xC0;
    assert(dnsReply(dns,19,sizeof(dns))==0);
    for(size_t n=0;n<19;++n) { memcpy(dns,original,sizeof(dns)); assert(!dnsReply(dns,n,sizeof(dns))); }
    // Exercise malformed DNS/HTTP lengths under sanitizers without trusting input.
    std::mt19937 random(123);
    for(unsigned trial=0;trial<3000;++trial) {
        size_t n=random()%513;
        for(size_t i=0;i<n;++i) dns[i]=uint8_t(random());
        assert(dnsReply(dns,n,sizeof(dns))<=sizeof(dns));
        Request r;
        for(unsigned i=0;i<1600;++i) r.feed(char(random()));
        assert(r.headers.size()<=Request::HeaderLimit && r.body.size()<=Request::BodyLimit);
    }
}
int main() {
    parsers();
    Credentials c; assert(!valid(c));
    strcpy(c.ssid,"  Family & Friends / \xC3\xA9  "); strcpy(c.password,"test-password"); assert(valid(c));
    strcpy(c.password,"short"); assert(!valid(c));
    memset(c.password,'f',64); c.password[64]=0; assert(valid(c));
    c.password[0]='g'; assert(!valid(c));
    c.password[0]=0; assert(valid(c)); // Open network.
    memset(c.ssid,'a',sizeof(c.ssid)); assert(!valid(c));
    char s1[33],s2[33]; uint8_t mac1[6]={1,2,3,4,5,6},mac2[6]={2,2,3,4,5,6};
    setupSsid(s1,mac1); setupSsid(s2,mac2); assert(strcmp(s1,s2));
    assert(strlen(s1)<=32);
    GatewayConfiguration::Config radio; radio.frequencyKhz=916500;
    assert(GatewayConfiguration::save(radio)); const std::string radioFlash=flash;
    // Replace the earlier long setup password without touching household/radio data.
    otherFlash["denclock-wifi/setup"] = std::string("ABCDEFGH23456789", 16) + char(0);
    Provisioning p; p.begin();
    assert(p.active && WiFi.ap && httpListening && dnsRunning && !p.savedValid);
    assert(!WiFi.persistentValue && !WiFi.autoReconnect);
    assert(strlen(p.password)==8 && WiFi.apPassword==p.password);
    for (const char ch : std::string(p.password))
        assert((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'));
    assert(otherFlash["denclock-wifi/setup"].size()==9);
    const std::string secret=p.password;
    char again[9]; uint8_t entropy[16]{};
    assert(setupPassword(again,entropy) && secret==again);
    WiFi.scanNames={"<script>&'", "Family & Friends"}; WiFi.scanValue=2;
    std::string page=exchange(p,"GET / HTTP/1.1\r\n\r\n");
    assert(page.find("&lt;script&gt;&amp;&#39;")!=std::string::npos);
    assert(page.find(secret)==std::string::npos);
    for(const char* path:{"/generate_204","/hotspot-detect.html","/connecttest.txt"}) {
        assert(exchange(p,std::string("GET ")+path+" HTTP/1.1\r\n\r\n").find("303 See Other")!=std::string::npos);
    }
    assert(post(p,"/connect","&ssid=Family+%26+Friends&password=wrong-password").find("Connecting")!=std::string::npos);
    assert(p.testing && WiFi.stationSsid=="Family & Friends");
    Credentials loaded; assert(!load(loaded));
    fakeMillis+=Provisioning::TestMs; p.tick();
    assert(!p.testing && p.active && !load(loaded));
    assert(strstr(p.message,"Connection failed"));
    post(p,"/connect","&network=46616D696C79202620467269656E6473&password=correct-password");
    assert(p.testing);
    WiFi.statusValue=WL_CONNECTED; WiFi.localAddress=0; p.tick();
    assert(p.testing && !load(loaded)); // Association alone is not verification.
    WiFi.localAddress=0xC0A8014D; p.tick();
    assert(p.success && load(loaded) && !strcmp(loaded.password,"correct-password"));
    page=exchange(p,"GET / HTTP/1.1\r\n\r\n");
    assert(page.find("192.168.1.77")!=std::string::npos && page.find("correct-password")==std::string::npos);
    fakeMillis+=14000; p.tick(); assert(p.active);
    fakeMillis+=1001; p.tick(); assert(!p.active && !WiFi.ap && !httpListening && !dnsRunning);
    // Simulated reboot uses saved credentials, no AP.
    Provisioning reboot; reboot.begin();
    assert(!reboot.active && reboot.savedValid && WiFi.stationSsid=="Family & Friends");
    fakeMillis+=Provisioning::RetryMs; reboot.tick(); assert(WiFi.reconnects>0);
    fakeMillis+=Provisioning::RecoveryMs; reboot.tick(); assert(reboot.active);
    // Manual entry is non-destructive and uses PRG only after firmware boot.
    reboot.stop(); WiFi.statusValue=WL_CONNECTED;
    buttonLevel=LOW; reboot.tick(); fakeMillis+=1999; reboot.tick(); assert(!reboot.active);
    ++fakeMillis; reboot.tick(); assert(reboot.active && load(loaded));
    buttonLevel=HIGH; reboot.tick();
    // Preserve old credentials on test timeout and storage failure.
    post(reboot,"/connect","&ssid=Bad&password=incorrect-password");
    fakeMillis+=Provisioning::TestMs; reboot.tick();
    assert(load(loaded) && !strcmp(loaded.ssid,"Family & Friends"));
    post(reboot,"/connect","&ssid=New&password=new-password");
    WiFi.statusValue=WL_CONNECTED; failSave=true; reboot.tick(); failSave=false;
    assert(!reboot.success && reboot.active && load(loaded) && !strcmp(loaded.ssid,"Family & Friends"));
    // CSRF token, malformed fields and GET queries cannot alter credentials.
    page=exchange(reboot,"POST /clear HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 7\r\n\r\ntoken=x");
    assert(page.find("Invalid setup request")!=std::string::npos && load(loaded));
    post(reboot,"/connect","&ssid="+std::string(33,'a')+"&password=password"); assert(!reboot.testing);
    exchange(reboot,"GET /clear?confirm=yes HTTP/1.1\r\n\r\n"); assert(load(loaded));
    post(reboot,"/clear",""); assert(load(loaded));
    post(reboot,"/clear","&confirm=yes");
    assert(!load(loaded) && flash==radioFlash && reboot.active);
    assert(secret==reboot.password);
    // HTTP-level rejection and retry, not only parser-unit checks.
    assert(exchange(reboot,"GET / HTTP/1.1\r\nX: "+std::string(1100,'x')+"\r\n\r\n").find("oversized")!=std::string::npos);
    assert(exchange(reboot,"GET / HTTP/1.1\r\nHost: attacker.example\r\n\r\n").find("303 See Other")!=std::string::npos);
    // Rollover-safe credential test and success-page grace timing.
    fakeMillis=UINT32_MAX-1000;
    post(reboot,"/connect","&ssid=Rollover&password=rollover-password");
    fakeMillis+=Provisioning::TestMs-100; reboot.tick(); assert(reboot.testing);
    fakeMillis+=101; reboot.tick(); assert(!reboot.testing && reboot.active);
    post(reboot,"/connect","&ssid=Retry&password=retry-password");
    WiFi.statusValue=WL_CONNECTED; reboot.tick(); assert(reboot.success);
    exchange(reboot,"GET / HTTP/1.1\r\n\r\n");
    WiFi.statusValue=0; reboot.tick(); fakeMillis+=16000; reboot.tick();
    assert(reboot.active && !reboot.success); // Link loss cancels AP shutdown.
    // Slow/oversized browser requests are bounded, then another browser works.
    int pair[2]; assert(socketpair(AF_UNIX,SOCK_STREAM,0,pair)==0); pendingBrowser=pair[0];
    const std::string slow="GET / HTTP/1.1\r\nX: "+std::string(800,'a');
    assert(send(pair[1],slow.data(),slow.size(),0)==ssize_t(slow.size()));
    const unsigned beforeReads=browserReads;
    reboot.tick();
    assert(browserReads-beforeReads==128);
    fakeMillis+=3000; reboot.tick(); ::close(pair[1]);
    assert(exchange(reboot,"GET / HTTP/1.1\r\n\r\n").find("DenClock Gateway")!=std::string::npos);
    reboot.stop();
    // A storage/AP failure never produces an unprotected setup network.
    assert(clear());
    otherFlash.erase("denclock-wifi/setup"); failSave=true;
    Provisioning broken; broken.begin(); assert(!broken.active && !WiFi.ap); failSave=false;
    fakeMillis+=5000; broken.tick(); assert(broken.active && strlen(broken.password)==8);
    broken.stop(); WiFi.apOk=false;
    Provisioning apFailure; apFailure.begin(); assert(!apFailure.active && !WiFi.ap);
    WiFi.apOk=true; fakeMillis+=5000; apFailure.tick(); assert(apFailure.active);
    assert(flash==radioFlash);
    assert(Serial.output.find("correct-password")==std::string::npos);
    assert(Serial.output.find(secret)==std::string::npos);
    std::cout<<"Wi-Fi provisioning tests passed\n";
}
