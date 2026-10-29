#include "dsp/Engine.h"
#include "util/Metrics.h"
#include "util/SignalGen.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

TEST_CASE("T2 null at Amount 0", "[T2][null]")
{
    constexpr double fs = 48000.0;
    constexpr int n = 48000;
    wg::dsp::Engine eng;
    wg::dsp::Spec spec{ fs, 512, 2 };
    wg::dsp::EngineParams p{};
    p.amount = 0.0f;
    p.autoGain = wg::dsp::AutoGainMode::Off;
    p.mix = 1.0f;
    p.osFactor = 4;
    eng.setTargets(p);
    eng.prepare(spec);

    std::vector<float> L(n), R(n), inL(n), inR(n);
    wg::test::fillSine(L, fs, 1000.0, -12.0);
    R = L;
    inL = L;
    inR = R;

    for (int off = 0; off < n;)
    {
        const int bs = std::min(512, n - off);
        float* c2[2] = { L.data() + off, R.data() + off };
        eng.process(c2, 2, bs);
        off += bs;
    }

    const int lat = eng.latencySamples();
    const int settle = static_cast<int>(0.05 * fs) + lat + 16;
    double peak = 0.0;
    for (int i = settle; i < n; ++i)
    {
        const int j = i - lat;
        if (j < 0)
            continue;
        peak = std::max(peak, static_cast<double>(std::abs(L[static_cast<std::size_t>(i)] - inL[static_cast<std::size_t>(j)])));
        peak = std::max(peak, static_cast<double>(std::abs(R[static_cast<std::size_t>(i)] - inR[static_cast<std::size_t>(j)])));
    }
    const double db = wg::test::lin2dB(peak);
    INFO("latency=" << lat << " peakDiffDb=" << db);
    REQUIRE(db < -120.0);
}

TEST_CASE("T8 determinism two engines", "[T8][determinism]")
{
    constexpr double fs = 48000.0;
    constexpr int n = 8192;
    auto run = [&](wg::dsp::Engine& e) {
        std::vector<float> L(n), R(n);
        wg::test::fillSine(L, fs, 440.0, -6.0);
        R = L;
        float* ch[2] = { L.data(), R.data() };
        e.process(ch, 2, n);
        return L;
    };

    wg::dsp::Spec spec{ fs, 512, 2 };
    wg::dsp::EngineParams p{};
    p.amount = 0.5f;
    p.autoGain = wg::dsp::AutoGainMode::Off;
    p.osFactor = 4;

    wg::dsp::Engine a, b;
    a.setTargets(p);
    b.setTargets(p);
    a.prepare(spec);
    b.prepare(spec);
    REQUIRE(run(a) == run(b));
}
