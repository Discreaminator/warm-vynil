#pragma once

#include "EngineParams.h"

#include <cstddef>
#include <cstdint>

namespace wg::dsp {

struct Spec
{
    double sampleRate = 48000.0;
    int maxBlock = 512;
    int numChannels = 2;
};

/** Empty passthrough engine for M0. Real chain arrives in M1. */
class Engine
{
public:
    void prepare(const Spec& spec) noexcept;
    void reset() noexcept;
    void setTargets(const EngineParams& params) noexcept;

    /** In-place passthrough. channels[c][n], numChannels = 1 or 2. */
    void process(float* const* channels, int numChannels, int numSamples) noexcept;

    [[nodiscard]] int latencySamples() const noexcept { return latencySamples_; }
    [[nodiscard]] const Spec& spec() const noexcept { return spec_; }

private:
    Spec spec_{};
    EngineParams targets_{};
    int latencySamples_ = 0;
};

} // namespace wg::dsp
