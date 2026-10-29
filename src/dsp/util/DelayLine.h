#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace wg::dsp {

/** Fractional delay line (linear interpolation). Buffer size is power-of-two when possible. */
class DelayLine
{
public:
    DelayLine() = default;

    void prepare(std::size_t maxDelaySamples)
    {
        const std::size_t minSize = maxDelaySamples + 2u;
        std::size_t size = 1u;
        while (size < minSize)
            size <<= 1u;
        buffer_.assign(size, 0.0f);
        mask_ = size - 1u;
        writeIndex_ = 0u;
    }

    void reset() noexcept
    {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        writeIndex_ = 0u;
    }

    [[nodiscard]] std::size_t maxDelay() const noexcept { return mask_; }

    void writeSample(float x) noexcept
    {
        buffer_[writeIndex_] = x;
        writeIndex_ = (writeIndex_ + 1u) & mask_;
    }

    [[nodiscard]] float readSample(float delaySamples) const noexcept
    {
        const float d = std::max(delaySamples, 0.0f);
        const float readPos = static_cast<float>(writeIndex_) - d;
        const auto i0 = static_cast<std::size_t>(std::floor(readPos)) & mask_;
        const auto i1 = (i0 + 1u) & mask_;
        const float frac = readPos - std::floor(readPos);
        return buffer_[i0] + frac * (buffer_[i1] - buffer_[i0]);
    }

    [[nodiscard]] float processSample(float x, float delaySamples) noexcept
    {
        const float y = readSample(delaySamples);
        writeSample(x);
        return y;
    }

    void processBlock(float* samples, int numSamples, float delaySamples) noexcept
    {
        for (int n = 0; n < numSamples; ++n)
            samples[n] = processSample(samples[n], delaySamples);
    }

private:
    std::vector<float> buffer_{};
    std::size_t mask_ = 0u;
    std::size_t writeIndex_ = 0u;
};

} // namespace wg::dsp
