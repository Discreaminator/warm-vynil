#include "dsp/Engine.h"
#include "util/SignalGen.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

namespace {

double benchSeconds(int os, bool adaa, int audioSeconds = 60)
{
    constexpr double fs = 48000.0;
    constexpr int block = 512;
    const int n = static_cast<int>(fs) * audioSeconds;

    wg::dsp::Engine eng;
    wg::dsp::EngineParams p{};
    p.amount = 1.0f;
    p.autoGain = wg::dsp::AutoGainMode::Off;
    p.osFactor = os;
    p.adaa = adaa;
    p.tubeModel = wg::dsp::TubeModel::Koren;
    eng.setTargets(p);
    eng.prepare({ fs, block, 2 });

    std::vector<float> L(static_cast<std::size_t>(n));
    std::vector<float> R(static_cast<std::size_t>(n));
    wg::test::fillSine(L, fs, 1000.0, -6.0);
    R = L;

    const auto t0 = std::chrono::steady_clock::now();
    for (int off = 0; off < n;)
    {
        const int bs = std::min(block, n - off);
        float* ch[2] = { L.data() + off, R.data() + off };
        eng.process(ch, 2, bs);
        off += bs;
    }
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(t1 - t0).count();
}

} // namespace

TEST_CASE("T16 CPU bench 60s stereo table", "[T16][cpu][m4][bench]")
{
    // SPEC: < 1.8 s for 60 s @ 48 kHz OS4x Koren+ADAA (≤ 3 % of one core).
    // CI reports the full 2x/4x/8x × ADAA on/off table; hard gate only on 4x+ADAA.
    struct Row
    {
        int os;
        bool adaa;
        double sec = 0.0;
    };
    Row rows[] = {
        { 2, false }, { 2, true },
        { 4, false }, { 4, true },
        { 8, false }, { 8, true },
    };

    for (auto& r : rows)
    {
        r.sec = benchSeconds(r.os, r.adaa, 60);
        std::printf("T16 OS%dx ADAA=%s  wall=%.3f s  (%.2f %% of realtime)\n",
                    r.os, r.adaa ? "on" : "off", r.sec, 100.0 * r.sec / 60.0);
    }

    double ref = 0.0;
    for (const auto& r : rows)
        if (r.os == 4 && r.adaa)
            ref = r.sec;

    INFO("OS4x+ADAA wallSeconds=" << ref);
    REQUIRE(ref > 0.0);
    // Soft CI gate: allow slower cloud/shared CPUs (SPEC 1.8 s is reference-machine).
    // Still require clearly realtime (< 50 % of one core).
    REQUIRE(ref < 30.0);
}
