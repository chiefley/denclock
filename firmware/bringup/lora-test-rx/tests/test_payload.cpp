#include "../src/test_payload.h"
#include <cassert>
#include <cstring>

int main()
{
    TestReadings r{};
    auto parse = [&](const char* s) { return parseTestPayload(s, std::strlen(s), r); };
    assert(parse("T=23.45 C RH=51.20 % BAT=4.08 V"));
    assert(std::fabs(r.battery - 4.08f) < 0.001f);
    assert(std::fabs(r.temperature - 23.45f) < 0.001f);
    assert(std::fabs(r.humidity - 51.20f) < 0.001f);
    assert(parse("T=-40.00 C RH=0.00 % BAT=0.00 V"));
    assert(parse("T=125.00 C RH=100.00 % BAT=4.20 V"));
    assert(parse("T=nan C RH=nan % BAT=3.80 V"));
    assert(std::isnan(r.temperature) && std::isnan(r.humidity));
    const char* invalid[] = {
        "", "DEN TEST 0", "T=23 C RH=50 %", "T=23 C RH=50 % BAT=4",
        "T=23 C RH=50 % BAT=4 V garbage", "T=bad C RH=50 % BAT=4 V",
        "T=23 F RH=50 % BAT=4 V", "T=23 C RH=101 % BAT=4 V",
        "T=-41 C RH=50 % BAT=4 V", "T=inf C RH=50 % BAT=4 V",
        "T=nan C RH=50 % BAT=4 V", "T=23 C RH=nan % BAT=4 V",
        "T=23 C RH=-1 % BAT=4 V", "T=23 C RH=inf % BAT=4 V",
        "T=23 C RH=50 % BAT=nan V", "T=23 C RH=50 % BAT=inf V",
        "T=23 C RH=50 % BAT=-1 V", "T=23 C RH=50 % BAT=7 V"
    };
    for (const char* s : invalid)
    {
        r = {1, 2, 3};
        assert(!parse(s));
        assert(r.temperature == 1 && r.humidity == 2 && r.battery == 3);
    }
    const char embedded[] = "T=23 C RH=50 % BAT=4 V\0junk";
    assert(!parseTestPayload(embedded, sizeof(embedded) - 1, r));
}
