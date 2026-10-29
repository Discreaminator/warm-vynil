#include "dsp/Engine.h"
#include "util/Metrics.h"
#include "util/SignalGen.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

TEST_CASE("T5 aliasing OS4x no ADAA M1 gate -80 dB", "[T5][alias]")
{
    constexpr double fs = 48000.0;
    const double tones[] = { 10000, 12000, 14000, 16000, 18000, 20000 };
    double worst = -999.0;

    for (double f : tones)
    {
        const int n = static_cast<int>(fs * 2);
        wg::dsp::Engine eng;
        wg::dsp::Spec spec{ fs, 512, 1 };
        wg::dsp::EngineParams p{};
        p.amount = 1.0f;
        p.autoGain = wg::dsp::AutoGainMode::Off;
        p.osFactor = 4;
        p.adaa = false;
        eng.setTargets(p);
        eng.prepare(spec);

        std::vector<float> buf(static_cast<std::size_t>(n));
        const double f0 = wg::test::coherentFreq(f, fs);
        wg::test::fillSine(buf, fs, f0, -6.0);
        for (int off = 0; off < n;)
        {
            const int bs = std::min(512, n - off);
            float* c = buf.data() + off;
            eng.process(&c, 1, bs);
            off += bs;
        }
        const int settle = static_cast<int>(0.5 * fs);
        auto mag = wg::test::magnitudeSpectrum(buf.data() + settle, n - settle);
        const double alias = wg::test::measureAliasing(mag, f0, fs);
        INFO("f=" << f0 << " aliasDb=" << alias);
        worst = std::max(worst, alias);
    }
    INFO("worstAliasDb=" << worst);
    // M1 DoD: SPEC §12 / milestone — 4x without ADAA < -80 dB.
    REQUIRE(worst < -80.0);
}
