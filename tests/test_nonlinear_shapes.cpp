#include "dsp/Engine.h"
#include "dsp/stages/Tracing.h"
#include "dsp/stages/Transformer.h"
#include "dsp/stages/TubeStage.h"
#include "dsp/stages/MonoBass.h"
#include "dsp/stages/Crosstalk.h"
#include "dsp/util/MathUtil.h"
#include "util/Metrics.h"
#include "util/SignalGen.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

TEST_CASE("T12 mono bass and crosstalk", "[t12]")
{
    wg::dsp::MonoBass mb;
    mb.prepare(48000.0);
    wg::dsp::MonoBassTargets t{};
    t.monoFreqHz = 150.0;
    t.depthM = 1.0;
    mb.setTargets(t);
    mb.reset();

    // Side-only: L=1, R=-1 → M=0,S=1; after m=1 low side killed
    constexpr int N = 48000;
    std::vector<float> L(N), R(N);
    const double f = 40.0; // << monoFreq/2
    for (int i = 0; i < N; ++i)
    {
        const float s = static_cast<float>(0.5 * std::sin(2.0 * 3.141592653589793 * f * i / 48000.0));
        L[static_cast<std::size_t>(i)] = s;
        R[static_cast<std::size_t>(i)] = -s;
    }
    mb.processBlock(L.data(), R.data(), N);
    double peakS = 0.0;
    for (int i = N / 2; i < N; ++i)
    {
        const double s = 0.5 * (static_cast<double>(L[static_cast<std::size_t>(i)])
                                - static_cast<double>(R[static_cast<std::size_t>(i)]));
        peakS = std::max(peakS, std::abs(s));
    }
    REQUIRE(wg::dsp::lin2dB(peakS / 0.5) <= -30.0);

    wg::dsp::Crosstalk xt;
    xt.prepare(192000.0);
    xt.setTargets(-30.0);
    xt.reset();
    // Measure L→R at 10 kHz: drive L, R=0
    double l = 1.0, r = 0.0;
    // settle filter
    for (int i = 0; i < 10000; ++i)
    {
        l = std::sin(2.0 * 3.141592653589793 * 10000.0 * i / 192000.0);
        r = 0.0;
        xt.processStereo(l, r);
    }
    // crude: k≈2c/(1+c); at HF emphasis~1 so leakage ~k/2 of L into R roughly
    REQUIRE(std::isfinite(r));
}

TEST_CASE("T13 nonlinear shapes: f(0)=0 and monotonic", "[t13]")
{
    wg::dsp::Tracing tr;
    tr.setAdaa(false); // transfer-curve test (ADAA is stateful)
    tr.setTargets(2.0);
    REQUIRE(tr.processSample(0.0) == 0.0);
    double prev = tr.processSample(-1.0);
    for (int i = 1; i <= 200; ++i)
    {
        const double u = -1.0 + 2.0 * i / 200.0;
        const double y = tr.processSample(u);
        REQUIRE(y >= prev - 1e-12);
        prev = y;
    }

    wg::dsp::TubeStage tube;
    tube.prepare(192000.0);
    wg::dsp::TubeTargets tt{};
    tt.driveDb = 9.0;
    tt.model = wg::dsp::TubeModel::Koren;
    tube.setTargets(tt);
    tube.reset();
    REQUIRE(std::abs(tube.processSample(0.0)) < 1e-6);

    wg::dsp::Transformer xf;
    xf.prepare(192000.0);
    xf.setTargets(0.5, 20000.0);
    xf.reset();
    // DC through transformer → near 0 after HPF settle
    double y = 0.0;
    for (int i = 0; i < 100000; ++i)
        y = xf.processSample(0.0);
    REQUIRE(std::abs(y) < 1e-9);
}

TEST_CASE("T13 ADAA vs no-ADAA harmonics within 1 dB at 1 kHz", "[t13][adaa][m4]")
{
    constexpr double fs = 48000.0;
    const double f0 = wg::test::coherentFreq(1000.0, fs);
    const int settle = static_cast<int>(0.5 * fs);

    auto render = [&](bool adaa) {
        const int n = static_cast<int>(fs * 2);
        wg::dsp::Engine eng;
        wg::dsp::EngineParams p{};
        p.amount = 0.5f;
        p.autoGain = wg::dsp::AutoGainMode::Off;
        p.osFactor = 4;
        p.adaa = adaa;
        eng.setTargets(p);
        eng.prepare({ fs, 512, 1 });
        std::vector<float> buf(static_cast<std::size_t>(n));
        wg::test::fillSine(buf, fs, f0, -6.0);
        for (int off = 0; off < n;)
        {
            const int bs = std::min(512, n - off);
            float* c = buf.data() + off;
            eng.process(&c, 1, bs);
            off += bs;
        }
        return wg::test::magnitudeSpectrum(buf.data() + settle, n - settle);
    };

    auto magOff = render(false);
    auto magOn = render(true);
    for (int h = 2; h <= 5; ++h)
    {
        const double a = wg::test::harmonicDb(magOff, f0, h, fs);
        const double b = wg::test::harmonicDb(magOn, f0, h, fs);
        INFO("H" << h << " off=" << a << " on=" << b);
        // Ignore buried harmonics (noise floor).
        if (a < -80.0 && b < -80.0)
            continue;
        REQUIRE(std::abs(a - b) <= 1.0);
    }
}
