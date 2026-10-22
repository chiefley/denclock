#pragma once
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>

namespace GatewayWiFi {
// One bounded HTTP/1 request; no chunking, uploads, pipelining or GET secrets.
struct Request {
    static constexpr size_t HeaderLimit = 1024, BodyLimit = 512;
    std::string headers, body, method, path, host;
    size_t contentLength = 0;
    bool parsed = false, done = false, bad = false;
    void feed(char c) {
        if (done || bad) return;
        if (!parsed) {
            if (headers.size() >= HeaderLimit) { bad = true; return; }
            headers += c;
            if (headers.size() >= 4 && headers.compare(headers.size()-4, 4, "\r\n\r\n") == 0) parse();
        } else {
            body += c;
            done = body.size() == contentLength;
        }
    }
    void parse() {
        parsed = true;
        size_t end = headers.find("\r\n"), a = headers.find(' '), b = headers.find(' ', a+1);
        if (a == std::string::npos || b == std::string::npos || b >= end || a > 8 || b-a > 96) { bad = true; return; }
        method = headers.substr(0,a); path = headers.substr(a+1,b-a-1);
        const std::string protocol = headers.substr(b+1,end-b-1);
        if ((method != "GET" && method != "POST") || (protocol != "HTTP/1.0" && protocol != "HTTP/1.1")) { bad = true; return; }
        bool lengthSeen = false, typeOk = false;
        size_t start = end+2;
        while (start+2 < headers.size()) {
            end = headers.find("\r\n",start);
            const size_t colon = headers.find(':',start);
            if (end == std::string::npos || colon == std::string::npos || colon >= end) { bad = true; return; }
            std::string key = headers.substr(start,colon-start);
            size_t v = colon+1;
            while (v < end && headers[v] == ' ') ++v;
            const std::string value = headers.substr(v,end-v);
            if (!strcasecmp(key.c_str(),"Host")) {
                if (!host.empty() || value.empty()) { bad=true; return; }
                host=value;
            } else if (!strcasecmp(key.c_str(),"Content-Length")) {
                if (lengthSeen || value.empty() || value.size() > 3) { bad = true; return; }
                lengthSeen = true;
                for (char ch : value) {
                    if (ch < '0' || ch > '9') { bad = true; return; }
                    contentLength = contentLength*10 + size_t(ch-'0');
                }
            } else if (!strcasecmp(key.c_str(),"Transfer-Encoding")) { bad = true; return; }
            else if (!strcasecmp(key.c_str(),"Content-Type")) typeOk = value == "application/x-www-form-urlencoded";
            start = end+2;
        }
        bad = contentLength > BodyLimit || (method == "POST" && (!lengthSeen || !typeOk)) ||
              (method == "GET" && contentLength != 0);
        done = !bad && contentLength == 0;
    }
};
inline int unhex(char c) {
    if (c >= '0' && c <= '9') return c-'0';
    if (c >= 'A' && c <= 'F') return c-'A'+10;
    if (c >= 'a' && c <= 'f') return c-'a'+10;
    return -1;
}
inline bool decode(const std::string& text, std::string& value, size_t limit) {
    value.clear();
    for (size_t i=0; i<text.size(); ++i) {
        char c = text[i];
        if (c == '%') {
            if (i+2 >= text.size()) return false;
            int a=unhex(text[i+1]), b=unhex(text[i+2]);
            if (a<0 || b<0) return false;
            c=char(a*16+b); i+=2;
        } else if (c == '+') c=' ';
        if (!c || value.size() == limit) return false;
        value += c;
    }
    return true;
}
// Reject duplicate/unknown fields, preventing ambiguous credentials or actions.
inline bool fields(const std::string& body, std::string (&values)[5]) {
    const char* names[] = {"token", "ssid", "network", "password", "confirm"};
    bool seen[5]{};
    size_t start=0;
    while (start < body.size()) {
        size_t end=body.find('&',start);
        if (end==std::string::npos) end=body.size();
        size_t eq=body.find('=',start);
        if (eq==std::string::npos || eq>=end) return false;
        const std::string key=body.substr(start,eq-start);
        unsigned n=0;
        while (n<5 && key!=names[n]) ++n;
        if (n==5 || seen[n] || !decode(body.substr(eq+1,end-eq-1),values[n], 64)) return false;
        seen[n]=true; start=end+1;
    }
    return seen[0];
}
inline std::string escape(const std::string& text) {
    std::string out;
    for (unsigned char c : text) {
        switch(c) {
        case '&': out+="&amp;"; break;
        case '<': out+="&lt;"; break;
        case '>': out+="&gt;"; break;
        case '"': out+="&quot;"; break;
        case '\'': out+="&#39;"; break;
        default:
            if (c < 32 || c >= 127) out+='?'; // Scan labels need not assume UTF-8.
            else out+=char(c);
        }
    }
    return out;
}
inline std::string hexSsid(const char* bytes, size_t length) {
    std::string out;
    const char* hex="0123456789ABCDEF";
    for (size_t i=0;i<length;++i) { unsigned char c=bytes[i]; out+=hex[c>>4]; out+=hex[c&15]; }
    return out;
}
} // namespace GatewayWiFi
