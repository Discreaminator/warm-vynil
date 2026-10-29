#pragma once

#include "dsp/util/MathUtil.h"

#include <cmath>

namespace wg::dsp {

/** Linear ramp toward target in native units (SPEC §8.2). Completes in setTimeMs. */
class LinearSmoother
{
public:
    void reset(double value = 0.0) noexcept
    {
        current_ = value;
        target_ = value;
        step_ = 0.0;
        remaining_ = 0;
    }

    void setTarget(double target) noexcept
    {
        target_ = target;
        if (rampSamples_ <= 1)
        {
            current_ = target_;
            step_ = 0.0;
            remaining_ = 0;
            return;
        }
        remaining_ = rampSamples_;
        step_ = (target_ - current_) / static_cast<double>(rampSamples_);
    }

    void setTimeMs(double sampleRate, double timeMs) noexcept
    {
        if (sampleRate <= 0.0 || timeMs <= 0.0)
        {
            rampSamples_ = 1;
            return;
        }
        rampSamples_ = std::max(1, static_cast<int>(std::lround(sampleRate * timeMs * 0.001)));
    }

    [[nodiscard]] double processSample() noexcept
    {
        if (remaining_ > 0)
        {
            current_ += step_;
            --remaining_;
            if (remaining_ == 0)
                current_ = target_;
        }
        else
        {
            current_ = target_;
        }
        return current_;
    }

    void skip(int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            (void)processSample();
    }

    [[nodiscard]] double current() const noexcept { return current_; }
    [[nodiscard]] double target() const noexcept { return target_; }

private:
    double current_ = 0.0;
    double target_ = 0.0;
    double step_ = 0.0;
    int remaining_ = 0;
    int rampSamples_ = 1;
};

/** Gain smoother: targets and state in dB, output is linear gain (SPEC §4.1, §8.2). */
class DbSmoother
{
public:
    void reset(double db = 0.0) noexcept { db_.reset(db); }

    void setTargetDb(double db) noexcept { db_.setTarget(db); }

    void setTimeMs(double sampleRate, double timeMs) noexcept { db_.setTimeMs(sampleRate, timeMs); }

    [[nodiscard]] double processSample() noexcept { return dB2lin(db_.processSample()); }

    void skip(int numSamples) noexcept { db_.skip(numSamples); }

    [[nodiscard]] double currentDb() const noexcept { return db_.current(); }

private:
    LinearSmoother db_{};
};

} // namespace wg::dsp
