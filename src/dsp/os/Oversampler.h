#pragma once

#include "dsp/os/HalfbandCoeffs.generated.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace wg::dsp::os {

/** Effective OS factor N from host fs and user choice (SPEC §6.1). */
[[nodiscard]] inline int effectiveOsFactor(double sampleRate, int osFactorChoice) noexcept
{
    int os = osFactorChoice;
    if (os != 2 && os != 4 && os != 8)
        os = 4;

    const double r = (sampleRate > 0.0) ? std::max(1.0, std::round(sampleRate / 48000.0)) : 1.0;
    int N = static_cast<int>(std::max(1.0, static_cast<double>(os) / r));
    if (N >= 8)
        N = 8;
    else if (N >= 4)
        N = 4;
    else if (N >= 2)
        N = 2;
    else
        N = 1;

    auto fsiOk = [&](int n) { return sampleRate * static_cast<double>(n); };
    while (N < 8 && fsiOk(N) < 88200.0)
        N = (N <= 1) ? 2 : (N == 2) ? 4 : 8;
    while (N > 1 && fsiOk(N) > 384000.0)
        N = (N == 8) ? 4 : (N == 4) ? 2 : 1;
    return N;
}

/** Cascaded half-band FIR oversampler, linear phase (SPEC §6.1). */
class Oversampler
{
public:
    void prepare(double sampleRate, int maxBlock, int osFactorChoice, int numChannels);

    void reset() noexcept;

    void upsample(int channel, const float* in, float* out, int numSamples) noexcept;
    void downsample(int channel, const float* in, float* out, int numSamples) noexcept;

    [[nodiscard]] int factor() const noexcept { return factor_; }
    [[nodiscard]] int latencySamples() const noexcept { return latencySamples_; }
    [[nodiscard]] double internalRate() const noexcept { return sampleRate_ * static_cast<double>(factor_); }

private:
    struct Stage
    {
        std::vector<double> coeffs;
        std::vector<double> zUp;
        std::vector<double> zDown;
        int length = 0;
        int stateLen = 0;
    };

    void buildStages(int numStages);
    void upsampleStage(Stage& st, int channel, const float* in, float* out, int nIn) noexcept;
    void downsampleStage(Stage& st, int channel, const float* in, float* out, int nOut) noexcept;

    double sampleRate_ = 48000.0;
    int factor_ = 4;
    int numChannels_ = 2;
    int latencySamples_ = 0;
    int maxBlock_ = 512;
    std::vector<Stage> stages_{};
    std::vector<float> scratch0_{};
    std::vector<float> scratch1_{};
};

} // namespace wg::dsp::os
