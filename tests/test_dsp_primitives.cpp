#include "dsp/Constants.h"
#include "dsp/Smoothing.h"
#include "dsp/filters/Biquad.h"
#include "dsp/filters/DcBlocker.h"
#include "dsp/filters/KWeighting.h"
#include "dsp/filters/LinkwitzRiley4.h"
#include "dsp/filters/OnePole.h"
#include "dsp/filters/SvfTpt.h"
#include "dsp/filters/TiltShelf.h"
#include "dsp/util/Adaa.h"
#include "dsp/util/DelayLine.h"
#include "dsp/util/Denormals.h"
#include "dsp/util/MathUtil.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

TEST_CASE("DSP util and filters compile and basic invariants", "[dsp][m1]")
{
    REQUIRE(wg::dsp::dB2lin(0.0) == Catch::Approx(1.0));
    REQUIRE(wg::dsp::smoothstep(0.5) == Catch::Approx(0.5));

    wg::dsp::DcBlocker<double> dc;
    dc.setSampleRate(48000.0);
    dc.reset();
    REQUIRE(dc.processSample(1.0) == Catch::Approx(1.0).margin(1.0e-9));

    wg::dsp::LinearSmoother smooth;
    smooth.reset(0.0);
    smooth.setTarget(1.0);
    smooth.setTimeMs(48000.0, 20.0);
    for (int i = 0; i < 5000; ++i)
        (void)smooth.processSample();
    REQUIRE(smooth.current() == Catch::Approx(1.0).margin(1.0e-3));

    wg::dsp::DelayLine delay;
    delay.prepare(64);
    delay.reset();
    delay.writeSample(1.0f);
    REQUIRE(delay.readSample(1.0f) == Catch::Approx(1.0f).margin(1.0e-6f));

    wg::dsp::LinkwitzRiley4 lr;
    lr.setCutoff(48000.0, 150.0);
    lr.reset();
    const double ap = lr.processAllpass2(0.25);
    REQUIRE(std::isfinite(ap));

    wg::dsp::KWeighting kw;
    kw.setSampleRate(48000.0);
    kw.reset();
    REQUIRE(std::isfinite(kw.processSample(0.1)));

    {
        wg::dsp::ScopedFlushDenormals flush;
        wg::dsp::TiltShelf tilt;
        tilt.setTiltDb(48000.0, 0.5);
        tilt.reset();
        REQUIRE(std::isfinite(tilt.processSample(0.1)));
    }

    wg::dsp::BiquadCoeffs play{};
    play.b0 = 1.1;
    play.b1 = -0.2;
    play.b2 = 0.05;
    play.a1 = -1.3;
    play.a2 = 0.4;
    const auto pre = wg::dsp::invertCoeffs(play);
    REQUIRE(pre.b0 == Catch::Approx(1.0));
}
