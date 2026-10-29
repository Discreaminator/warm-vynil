#include "dsp/Engine.h"
#include "dsp/EngineParams.h"
#include "util/SignalGen.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <vector>

TEST_CASE("T9 silence is exact zero; tiny input stays finite", "[t9]")
{
    wg::dsp::Engine e;
    wg::dsp::EngineParams p{};
    p.amount = 0.5f;
    p.autoGain = wg::dsp::AutoGainMode::Off;
    e.setTargets(p);
    e.prepare({ 48000.0, 512, 1 });
    e.reset();

    std::vector<float> z(4096, 0.0f);
    float* ch[1] = { z.data() };
    e.process(ch, 1, 4096);
    for (float v : z)
        REQUIRE(v == 0.0f);

    std::vector<float> tiny(4096, 1.0e-30f);
    ch[0] = tiny.data();
    e.process(ch, 1, 4096);
    for (float v : tiny)
        REQUIRE(std::isfinite(v));
}

TEST_CASE("T10 edge rates / NaN sanitize", "[t10]")
{
    for (double fs : { 22050.0, 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        INFO("fs=" << fs);
        wg::dsp::Engine e;
        wg::dsp::EngineParams p{};
        p.amount = 0.4f;
        p.autoGain = wg::dsp::AutoGainMode::Off;
        e.setTargets(p);
        e.prepare({ fs, 4096, 2 });
        auto sig = wg::test::sine(1000.0, fs, 4096, 0.1);
        std::vector<float> R = sig;
        float* ch[2] = { sig.data(), R.data() };
        e.process(ch, 2, 1);
        e.process(ch, 2, 4096);

        sig[10] = std::numeric_limits<float>::quiet_NaN();
        e.process(ch, 2, 64);
        for (int i = 0; i < 64; ++i)
            REQUIRE(std::isfinite(sig[static_cast<std::size_t>(i)]));
    }
}
