#include "dsp/stages/Riaa.h"
#include "dsp/stages/Tracing.h"
#include "dsp/util/MathUtil.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

TEST_CASE("T3 stage3 linear transparency", "[T3][riaa]")
{
    // SPEC T3: s_tr=0, tilt 0, Neumann off → linear stage [3] null < -120 dBFS.
    constexpr double fsi = 192000.0;
    constexpr int N = 48000;
    wg::dsp::Riaa riaa;
    riaa.prepare(fsi);
    riaa.setTargets(0.0, 0.0);
    riaa.reset();
    wg::dsp::Tracing tr;
    tr.setTargets(0.0);

    double errEnergy = 0.0;
    double sigEnergy = 0.0;
    for (int i = 0; i < N; ++i)
    {
        const double x = 0.5 * std::sin(2.0 * wg::dsp::kPi * 1000.0 * i / fsi);
        double y = riaa.processPreSample(x);
        y = riaa.processNeumannSample(y);
        y = tr.processSample(y);
        y = riaa.processPostChainSample(y);
        if (i > N / 4)
        {
            const double e = y - x;
            errEnergy += e * e;
            sigEnergy += x * x;
        }
    }
    const double ratio = std::sqrt(errEnergy / std::max(sigEnergy, 1.0e-30));
    REQUIRE(wg::dsp::lin2dB(ratio) < -120.0);
}
