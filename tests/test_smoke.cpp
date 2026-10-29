#include "dsp/Engine.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <vector>

TEST_CASE("Engine passthrough leaves buffer unchanged", "[smoke][m0]")
{
    wg::dsp::Engine engine;
    wg::dsp::Spec spec;
    spec.sampleRate = 48000.0;
    spec.maxBlock = 64;
    spec.numChannels = 2;
    engine.prepare(spec);

    constexpr int n = 64;
    std::vector<float> left(static_cast<size_t>(n));
    std::vector<float> right(static_cast<size_t>(n));
    std::vector<float> leftRef(static_cast<size_t>(n));
    std::vector<float> rightRef(static_cast<size_t>(n));

    for (int i = 0; i < n; ++i)
    {
        const float x = 0.25f * std::sin(2.0f * 3.14159265f * 1000.0f * static_cast<float>(i) / 48000.0f);
        left[static_cast<size_t>(i)] = x;
        right[static_cast<size_t>(i)] = -0.5f * x;
        leftRef[static_cast<size_t>(i)] = left[static_cast<size_t>(i)];
        rightRef[static_cast<size_t>(i)] = right[static_cast<size_t>(i)];
    }

    float* chans[2] = { left.data(), right.data() };
    engine.process(chans, 2, n);

    for (int i = 0; i < n; ++i)
    {
        REQUIRE(left[static_cast<size_t>(i)] == leftRef[static_cast<size_t>(i)]);
        REQUIRE(right[static_cast<size_t>(i)] == rightRef[static_cast<size_t>(i)]);
    }

    REQUIRE(engine.latencySamples() == 0);
}
