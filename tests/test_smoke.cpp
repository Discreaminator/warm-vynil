#include "dsp/Engine.h"
#include "dsp/EngineParams.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

TEST_CASE("Engine Amount 0 is delayed dry after settle (T2 smoke)", "[smoke][t2]")
{
    wg::dsp::Engine engine;
    wg::dsp::Spec spec;
    spec.sampleRate = 48000.0;
    spec.maxBlock = 512;
    spec.numChannels = 2;

    wg::dsp::EngineParams params{};
    params.amount = 0.0f;
    params.autoGain = wg::dsp::AutoGainMode::Off;
    params.mix = 1.0f;
    params.outGainDb = 0.0f;
    params.inGainDb = 0.0f;
    engine.setTargets(params);
    engine.prepare(spec);

    const int latency = engine.latencySamples();
    const int settle = static_cast<int>(0.06 * spec.sampleRate); // > 50 ms
    const int n = settle + latency + 256;

    std::vector<float> left(static_cast<size_t>(n));
    std::vector<float> right(static_cast<size_t>(n));
    std::vector<float> leftIn(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i)
    {
        const float s = 0.25f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / 48000.0f);
        left[static_cast<size_t>(i)] = s;
        right[static_cast<size_t>(i)] = s * 0.5f;
        leftIn[static_cast<size_t>(i)] = s;
    }

    float* ch[2] = { left.data(), right.data() };
    // Process in irregular blocks
    int off = 0;
    while (off < n)
    {
        const int block = std::min(64, n - off);
        float* p[2] = { left.data() + off, right.data() + off };
        engine.process(p, 2, block);
        off += block;
    }

    // After settle, output must match delayed input bit-exactly (or < -120 dBFS)
    double maxDiff = 0.0;
    for (int i = settle; i < n; ++i)
    {
        const int src = i - latency;
        if (src < 0)
            continue;
        const double d = std::abs(static_cast<double>(left[static_cast<size_t>(i)])
                                  - static_cast<double>(leftIn[static_cast<size_t>(src)]));
        maxDiff = std::max(maxDiff, d);
    }
    const double diffDb = (maxDiff > 0.0) ? 20.0 * std::log10(maxDiff) : -300.0;
    REQUIRE(diffDb < -120.0);
}
