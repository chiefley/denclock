#pragma once
#include <WiFi.h>
#include "setup_dns.h"
#include <esp_system.h>
#include <sys/socket.h>
#include <errno.h>
#include "wifi_settings.h"
#include "setup_http.h"

namespace GatewayWiFi {
class Provisioning {
public:
    static constexpr uint32_t RecoveryMs=60000, TestMs=30000, RetryMs=5000;
    bool active=false, testing=false, success=false, savedValid=false;
    char ssid[33]{}, password[9]{}, token[33]{};
    const char* message="Choose your household Wi-Fi network.";
    Credentials saved{}, candidate{};
    uint32_t attemptAt=0, offlineAt=0, retryAt=0, successAt=0;
    bool offline=false;
private:
    WiFiServer server{IPAddress(192,168,4,1), 80, 1};
    SetupDns dns;
    WiFiClient browser;
    Request request;
    std::string response;
    size_t sent=0;
    uint32_t browserAt=0, deliveredAt=0, buttonAt=0, apRetryAt=0;
    bool responseSuccess=false, browserOpen=false, delivered=false, buttonDown=false, buttonHandled=false;
    bool scanRunning=false;
    std::string networks;
    void dropBrowser() {
        browser.stop(); browserOpen=false;
        request=Request{}; response.clear(); sent=0;
    }
    void reply(const std::string& body, bool redirect=false) {
        response="HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\n";
        response+="Content-Security-Policy: default-src 'none'; style-src 'unsafe-inline'; form-action http://192.168.4.1; base-uri 'none'; frame-ancestors 'none'\r\n";
        if (redirect) response="HTTP/1.1 303 See Other\r\nLocation: http://192.168.4.1/\r\nCache-Control: no-store\r\nConnection: close\r\n";
        response+="Content-Length: "+std::to_string(body.size())+"\r\n\r\n"+body;
        sent=0; responseSuccess=false;
    }
    void page() {
        std::string p="<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
        if (testing || scanRunning) p+="<meta http-equiv='refresh' content='3;url=http://192.168.4.1/'>";
        p+="<title>DenClock Wi-Fi setup</title><style>body{font:18px system-ui;max-width:32em;margin:2em auto;padding:1em}input,select,button{font:inherit;box-sizing:border-box;width:100%;padding:.6em;margin:.4em 0}button{cursor:pointer}label{display:block}small{display:block}</style></head><body><h1>DenClock Gateway</h1><p>";
        p+=escape(ssid); p+="</p><p role='status'>"; p+=escape(message); p+="</p>";
        if (success) {
            p+="<p>Household IP: "+escape(WiFi.localIP().toString().c_str())+"</p><p>Setup will close shortly. Rejoin your household Wi-Fi network.</p>";
        } else if (!testing) {
            p+="<form method='post' action='http://192.168.4.1/connect'><input type='hidden' name='token' value='";
            p+=token; p+="'><label>Nearby networks<select name='network'><option value=''>Enter manually below</option>";
            p+=networks;
            p+="</select></label><label>SSID (manual entry overrides selection)<input name='ssid' maxlength='32' autocomplete='off'></label><label>Household Wi-Fi password<input name='password' type='password' maxlength='64' autocomplete='new-password'></label><small>Leave password empty for an open network. 2.4 GHz Wi-Fi only.</small><button>Connect</button></form>";
            p+="<form method='post' action='http://192.168.4.1/scan'><input type='hidden' name='token' value='";
            p+=token; p+="'><button>Scan again</button></form><details><summary>Clear saved household Wi-Fi</summary><form method='post' action='http://192.168.4.1/clear'><input type='hidden' name='token' value='";
            p+=token; p+="'><label><input type='checkbox' name='confirm' value='yes' required>Forget household Wi-Fi only</label><button>Clear Wi-Fi credentials</button></form></details>";
        } else p+="<p>If your phone disconnects during the channel change, reconnect to the same setup network and open http://192.168.4.1 again.</p>";
        p+="<p><a href='http://192.168.4.1/'>Refresh status</a></p></body></html>";
        reply(p); responseSuccess=success;
    }
    void scan() {
        WiFi.scanDelete(); networks.clear();
        scanRunning=WiFi.scanNetworks(true, true, false, 120) == WIFI_SCAN_RUNNING;
    }
    void scanTick() {
        if (!scanRunning) return;
        const int n=WiFi.scanComplete();
        if (n==WIFI_SCAN_RUNNING) return;
        scanRunning=false;
        for (int i=0; i<n && i<16; ++i) {
            const auto name=WiFi.SSID(i);
            if (!name.length() || name.length()>32) continue;
            networks+="<option value='"+hexSsid(name.c_str(),name.length())+"'>";
            networks+=escape(std::string(name.c_str(),name.length()));
            networks+=" ("+std::to_string(WiFi.RSSI(i))+" dBm)</option>";
        }
        WiFi.scanDelete();
    }
    void handle() {
        if (!request.host.empty() && request.host!="192.168.4.1" && request.host!="192.168.4.1:80") {
            reply("",true); return;
        }
        if (request.method=="GET") {
            // Query strings never participate in actions or credential handling.
            if (request.path=="/") page();
            else reply("",true); // Includes Android, Apple and Windows probes.
            return;
        }
        std::string v[5];
        if (!fields(request.body,v) || v[0]!=token) { reply("Invalid setup request."); return; }
        if (testing || success) { page(); return; }
        if (request.path=="/scan") scan();
        else if (request.path=="/clear") {
            if (v[4]!="yes") { reply("Confirm clearing household Wi-Fi."); return; }
            if (clear()) {
                saved=Credentials{}; savedValid=false; WiFi.disconnect(false,false);
                message="Household Wi-Fi cleared. Radio settings are unchanged.";
            } else message="Could not clear Wi-Fi storage. Try again.";
        } else if (request.path=="/connect") {
            candidate=Credentials{};
            std::string name=v[1];
            if (name.empty() && !v[2].empty()) {
                if (v[2].size()>64 || v[2].size()%2) { reply("Invalid network selection."); return; }
                for (size_t i=0;i<v[2].size();i+=2) {
                    int a=unhex(v[2][i]), b=unhex(v[2][i+1]);
                    if (a<0 || b<0 || a*16+b==0) { reply("Invalid network selection."); return; }
                    name+=char(a*16+b);
                }
            }
            if (name.empty() || name.size()>32 || v[3].size()>64) { reply("Invalid SSID or password length."); return; }
            memcpy(candidate.ssid,name.data(),name.size());
            memcpy(candidate.password,v[3].data(),v[3].size());
            if (!valid(candidate)) { reply("Use a 1–32 byte SSID and an empty password, 8–63 printable ASCII characters, or 64 hexadecimal digits."); return; }
            if (scanRunning) { reply("Scan in progress. Refresh and try again shortly."); return; }
            WiFi.disconnect(false,false);
            WiFi.begin(candidate.ssid,candidate.password);
            attemptAt=millis(); testing=true;
            message="Connecting to household Wi-Fi. This can take up to 30 seconds.";
        } else { reply("Unknown setup action."); return; }
        page();
    }
    void httpTick() {
        if (!browserOpen) {
            browser=server.available();
            if (!browser) return;
            browserOpen=true; browserAt=millis();
        }
        // Absolute deadlines stop slow headers/bodies and stalled readers.
        if (uint32_t(millis()-browserAt)>=3000 || !browser.connected()) { dropBrowser(); return; }
        if (response.empty()) {
            unsigned budget=128;
            while (budget-- && browser.available() && !request.done && !request.bad) {
                const int c=browser.read();
                if (c<0) break;
                request.feed(char(c));
            }
            if (request.bad) reply("Invalid or oversized HTTP request.");
            else if (request.done) handle();
        }
        if (!response.empty()) {
            const size_t length=std::min(size_t(256),response.size()-sent);
            // WiFiClient::write retries synchronously; use ESP32 lwIP's
            // nonblocking socket send so slow browsers cannot hold the loop.
            const int n=::send(browser.fd(),response.data()+sent,length,MSG_DONTWAIT);
            if (n>0) sent+=size_t(n);
            else if (n<0 && errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) { dropBrowser(); return; }
            if (sent==response.size()) {
                if (responseSuccess && success && !delivered) { delivered=true; deliveredAt=millis(); }
                dropBrowser();
            }
        }
    }
public:
    bool usable() { return WiFi.status()==WL_CONNECTED && uint32_t(WiFi.localIP())!=0; }
    void begin() {
        WiFi.persistent(false); // Only explicit verified credentials go to our NVS record.
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(false);
        pinMode(0,INPUT_PULLUP);
        uint8_t mac[6]; WiFi.macAddress(mac); setupSsid(ssid,mac);
        uint8_t random[16]; esp_fill_random(random,sizeof(random));
        // Failure to persist the setup secret never starts an open/fallback AP.
        if (!setupPassword(password,random)) message="Setup password storage failed; retrying.";
        savedValid=load(saved);
        offlineAt=retryAt=millis(); offline=true;
        if (savedValid) WiFi.begin(saved.ssid,saved.password);
        else start();
    }
    void start() {
        if (active) return;
        apRetryAt=millis();
        if (!password[0]) {
            uint8_t random[16]; esp_fill_random(random,sizeof(random));
            if (!setupPassword(password,random)) return;
        }
        WiFi.disconnect(false,false);
        WiFi.mode(WIFI_AP_STA);
        WiFi.setAutoReconnect(false);
        IPAddress ip; ip.fromString("192.168.4.1");
        IPAddress mask; mask.fromString("255.255.255.0");
        if (!WiFi.softAPConfig(ip,ip,mask) || !WiFi.softAP(ssid,password,1,false,2)) {
            WiFi.softAPdisconnect(true); WiFi.mode(WIFI_STA);
            message="Setup access point failed; retrying."; return;
        }
        uint8_t random[16]; esp_fill_random(random,sizeof(random));
        const std::string t=hexSsid(reinterpret_cast<char*>(random),sizeof(random));
        memcpy(token,t.c_str(),sizeof(token));
        if (!dns.start(ip)) {
            WiFi.softAPdisconnect(true); WiFi.mode(WIFI_STA);
            message="Setup DNS failed; retrying."; return;
        }
        server.begin();
        if (!server) {
            server.end(); dns.stop(); WiFi.softAPdisconnect(true); WiFi.mode(WIFI_STA);
            message="Setup HTTP failed; retrying."; return;
        }
        active=true; testing=success=delivered=false;
        message="Choose your household Wi-Fi network.";
        scan();
        Serial.println("Wi-Fi setup available; see OLED for access details.");
    }
    void stop() {
        dropBrowser(); server.end(); dns.stop(); WiFi.scanDelete(); scanRunning=false;
        WiFi.softAPdisconnect(true); WiFi.mode(WIFI_STA);
        active=false; success=false; offline=false;
        Serial.println("Wi-Fi setup complete; temporary access point stopped.");
    }
    void tick() {
        const uint32_t now=millis();
        const bool pressed=digitalRead(0)==LOW;
        if (!pressed) { buttonDown=buttonHandled=false; }
        else if (!buttonDown) { buttonDown=true; buttonAt=now; }
        else if (!buttonHandled && uint32_t(now-buttonAt)>=2000) { buttonHandled=true; start(); }
        if (active) {
            dns.tick(); scanTick();
            if (testing) {
                if (usable()) {
                    if (save(candidate)) {
                        saved=candidate; savedValid=true; success=true; successAt=now;
                        message="Connected and saved successfully.";
                    } else {
                        WiFi.disconnect(false,false);
                        message="Connected, but saving failed. Credentials unchanged; try again.";
                    }
                    testing=false; candidate=Credentials{};
                } else if (uint32_t(now-attemptAt)>=TestMs) {
                    WiFi.disconnect(false,false); testing=false; candidate=Credentials{};
                    message="Connection failed. Check password, network range and 2.4 GHz support; try again.";
                }
            }
            // A dropped household link during the success grace period is a
            // recoverable setup failure, not permission to shut down the AP.
            if (success && !usable()) {
                success=delivered=false; message="Household connection lost. Please try again.";
            }
            httpTick();
            const uint32_t afterHttp=millis();
            if (success && ((delivered && uint32_t(afterHttp-deliveredAt)>=15000) || uint32_t(afterHttp-successAt)>=60000)) stop();
            return;
        }
        if (usable()) { offline=false; return; }
        if (!offline) { offline=true; offlineAt=retryAt=now; }
        if (!savedValid || uint32_t(now-offlineAt)>=RecoveryMs) {
            if (uint32_t(now-apRetryAt)>=RetryMs) start();
        } else if (uint32_t(now-retryAt)>=RetryMs) { retryAt=now; WiFi.reconnect(); }
    }
};
} // namespace GatewayWiFi
