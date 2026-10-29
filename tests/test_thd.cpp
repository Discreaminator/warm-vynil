#include "dsp/Engine.h"
#include "util/Metrics.h"
#include "util/SignalGen.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace {

std::vector<float> renderTone(double fs, double freq, double levelDb, float amount, int seconds = 2)
{
    const int n = static_cast<int>(fs * seconds);
    wg::dsp::Engine eng;
    wg::dsp::Spec spec{ fs, 512, 1 };
    wg::dsp::EngineParams p{};
    p.amount = amount;
    p.autoGain = wg::dsp::AutoGainMode::Off;
    p.osFactor = 4;
    p.mix = 1.0f;
    p.riaaNeumann = false;
    p.riaaTilt = 0.0f;
    p.cartFreq = 18000.0f;
    p.cartQ = 0.707f;
    p.xfmrSat = 0.2f;
    eng.setTargets(p);
    eng.prepare(spec);

    std::vector<float> buf(static_cast<std::size_t>(n));
    const double f0 = wg::test::coherentFreq(freq, fs);
    wg::test::fillSine(buf, fs, f0, levelDb);
    for (int off = 0; off < n;)
    {
        const int bs = std::min(512, n - off);
        float* c = buf.data() + off;
        eng.process(&c, 1, bs);
        off += bs;
    }
    return buf;
}

} // namespace

TEST_CASE("T4 THD at a=0.5 -6dBFS", "[T4][thd]")
{
    constexpr double fs = 48000.0;
    auto buf = renderTone(fs, 1000.0, -6.0, 0.5f);
    const int settle = static_cast<int>(0.5 * fs);
    auto mag = wg::test::magnitudeSpectrum(buf.data() + settle, static_cast<int>(buf.size()) - settle);
    const double f0 = wg::test::coherentFreq(1000.0, fs);
    const double thd = wg::test::measureThd(mag, f0, fs);
    const double h2 = wg::test::harmonicDb(mag, f0, 2, fs);
    const double h3 = wg::test::harmonicDb(mag, f0, 3, fs);
    INFO("THD=" << thd * 100.0 << "% H2=" << h2 << " H3=" << h3);
    REQUIRE(thd >= 0.005);
    REQUIRE(thd <= 0.03);
    REQUIRE(h2 > h3 + 3.0);
}

TEST_CASE("T4b level dependence", "[T4b][thd]")
{
    constexpr double fs = 48000.0;
    auto hi = renderTone(fs, 1000.0, -6.0, 0.5f);
    auto lo = renderTone(fs, 1000.0, -20.0, 0.5f);
    const int settle = static_cast<int>(0.5 * fs);
    const double f0 = wg::test::coherentFreq(1000.0, fs);
    const double thdHi = wg::test::measureThd(
        wg::test::magnitudeSpectrum(hi.data() + settle, static_cast<int>(hi.size()) - settle), f0, fs);
    const double thdLo = wg::test::measureThd(
        wg::test::magnitudeSpectrum(lo.data() + settle, static_cast<int>(lo.size()) - settle), f0, fs);
    REQUIRE(wg::test::lin2dB(thdLo) <= wg::test::lin2dB(thdHi) - 6.0);
}

TEST_CASE("T4c THD monotone in Amount", "[T4c][thd]")
{
    constexpr double fs = 48000.0;
    const double f0 = wg::test::coherentFreq(1000.0, fs);
    const int settle = static_cast<int>(0.5 * fs);
    double prev = -1.0;
    for (int i = 0; i <= 10; ++i)
    {
        const float a = static_cast<float>(i) * 0.1f;
        auto buf = renderTone(fs, 1000.0, -6.0, a);
        const double thd = wg::test::measureThd(
            wg::test::magnitudeSpectrum(buf.data() + settle, static_cast<int>(buf.size()) - settle), f0, fs);
        INFO("a=" << a << " thd=" << thd);
        if (i == 0)
            REQUIRE(thd < 0.00001);
        else
            REQUIRE(thd + 1e-12 >= prev);
        prev = thd;
    }
}

TEST_CASE("T4d tracing frequency dependence", "[T4d][thd]")
{
    constexpr double fs = 48000.0;
    auto render = [&](double freq) {
        const int n = static_cast<int>(fs * 2);
        wg::dsp::Engine eng;
        wg::dsp::Spec spec{ fs, 512, 1 };
        wg::dsp::EngineParams p{};
        p.amount = 0.5f;
        p.autoGain = wg::dsp::AutoGainMode::Off;
        p.osFactor = 4;
        p.tubeTrim = 0.0f;
        p.xfmrSat = 0.0f;
        p.riaaTilt = 0.0f;
        p.riaaNeumann = false;
        p.cartFreq = 40000.0f; // open cart so H2@20 kHz isn't LP-killed
        p.cartQ = 0.707f;
        p.tracingTrim = 2.0f; // max trim: isolate HF tracing advantage (T4d)
        p.xfmrHF = 40000.0f;
        eng.setTargets(p);
        eng.prepare(spec);
        std::vector<float> buf(static_cast<std::size_t>(n));
        const double f0 = wg::test::coherentFreq(freq, fs);
        wg::test::fillSine(buf, fs, f0, -12.0);
        for (int off = 0; off < n;)
        {
            const int bs = std::min(512, n - off);
            float* c = buf.data() + off;
            eng.process(&c, 1, bs);
            off += bs;
        }
        return std::make_pair(buf, f0);
    };

    const int settle = static_cast<int>(0.5 * fs);
    auto [b1, f1] = render(1000.0);
    auto [b10, f10] = render(10000.0);
    const double h2_1 = wg::test::harmonicDb(
        wg::test::magnitudeSpectrum(b1.data() + settle, static_cast<int>(b1.size()) - settle), f1, 2, fs);
    const double h2_10 = wg::test::harmonicDb(
        wg::test::magnitudeSpectrum(b10.data() + settle, static_cast<int>(b10.size()) - settle), f10, 2, fs);
    INFO("H2_1k=" << h2_1 << " H2_10k=" << h2_10);
    REQUIRE(h2_10 - h2_1 >= 8.0);
}
