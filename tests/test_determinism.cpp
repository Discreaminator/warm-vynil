#include "dsp/Engine.h"
#include "util/SignalGen.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <vector>

TEST_CASE("T7 block-size independence constant params", "[T7][blocks]")
{
    constexpr double fs = 48000.0;
    constexpr int n = 48000;
    std::vector<float> src(static_cast<std::size_t>(n));
    {
        std::vector<float> t1(src.size()), t2(src.size()), t3(src.size());
        wg::test::fillSine(t1, fs, 220.0, -18.0);
        wg::test::fillSine(t2, fs, 1000.0, -20.0, 0.3);
        wg::test::fillSine(t3, fs, 5000.0, -24.0, 1.1);
        for (std::size_t i = 0; i < src.size(); ++i)
            src[i] = t1[i] + t2[i] + t3[i];
    }

    auto render = [&](const std::vector<int>& blocks) {
        wg::dsp::Engine eng;
        wg::dsp::Spec spec{ fs, 4096, 1 };
        wg::dsp::EngineParams p{};
        p.amount = 0.5f;
        p.autoGain = wg::dsp::AutoGainMode::Off;
        p.osFactor = 4;
        eng.setTargets(p);
        eng.prepare(spec);
        std::vector<float> buf = src;
        int off = 0;
        std::size_t bi = 0;
        while (off < n)
        {
            int bs = blocks.empty() ? 512 : blocks[bi % blocks.size()];
            bs = std::min(bs, n - off);
            float* c = buf.data() + off;
            eng.process(&c, 1, bs);
            off += bs;
            ++bi;
        }
        return buf;
    };

    const auto ref = render({ 512 });
    const std::vector<std::vector<int>> cases = {
        { 1 }, { 7 }, { 32 }, { 64 }, { 4096 }, { 1, 7, 32, 64, 13, 97 }
    };
    for (const auto& blocks : cases)
        REQUIRE(render(blocks) == ref);
}
