#pragma once

#include "dsp/filters/LinkwitzRiley4.h"
#include "dsp/util/MathUtil.h"

namespace wg::dsp {

struct MonoBassTargets
{
    double monoFreqHz = 150.0;
    double depthM = 0.0;
};

/** Elliptical mono bass on Side channel (SPEC §4.2). Bypass when numChannels == 1. */
class MonoBass
{
public:
    void prepare(double sampleRate) noexcept { fs_ = sampleRate; }

    void reset() noexcept
    {
        lrS_.reset();
        lrM_.reset();
    }

    void setTargets(const MonoBassTargets& t) noexcept
    {
        targets_ = t;
        if (fs_ > 0.0 && t.monoFreqHz > 0.0)
        {
            lrS_.setCutoff(fs_, t.monoFreqHz);
            lrM_.setCutoff(fs_, t.monoFreqHz);
        }
    }

    void processBlock(float* left, float* right, int numSamples) noexcept
    {
        const double m = clamp(targets_.depthM, 0.0, 1.0);
        for (int n = 0; n < numSamples; ++n)
        {
            const double L = static_cast<double>(left[n]);
            const double R = static_cast<double>(right[n]);
            const double M = 0.5 * (L + R);
            const double S = 0.5 * (L - R);

            const double sLow = lrS_.processLowpass(S);
            const double sHigh = lrS_.processHighpass(S);
            const double sOut = sHigh + (1.0 - m) * sLow;

            const double mOut = lrM_.processAllpass2(M);

            left[n] = static_cast<float>(mOut + sOut);
            right[n] = static_cast<float>(mOut - sOut);
        }
    }

private:
    double fs_ = 48000.0;
    MonoBassTargets targets_{};
    LinkwitzRiley4 lrS_{};
    LinkwitzRiley4 lrM_{};
};

} // namespace wg::dsp
