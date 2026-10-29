#pragma once

#include "TubeLut.h"
#include "dsp/EngineParams.h"

namespace wg::dsp {

struct TubeTargets
{
    double driveDb = 0.0;
    double tubeBias = 0.0; // −1…+1 user
    double kDyn = 0.0;
    TubeModel model = TubeModel::Koren;
    bool adaa = true;
};

/** 12AX7 stage: Koren LUT (default) or Fast tanh (SPEC §4.5), ADAA optional (M4). */
class TubeStage
{
public:
    void prepare(double fsi) noexcept;

    void reset() noexcept;

    void setTargets(const TubeTargets& t) noexcept;

    [[nodiscard]] double processSample(double x) noexcept;

    /** Shared LUT accessor for isolated tests. */
    [[nodiscard]] const TubeLut& lut() const noexcept { return lut_; }

private:
    [[nodiscard]] double processKoren(double x) noexcept;
    [[nodiscard]] double processFast(double x) noexcept;

    [[nodiscard]] static double lnCosh(double v) noexcept;

    double fsi_ = 48000.0;
    TubeTargets targets_{};
    TubeLut lut_{};
    double env_ = 0.0;
    double atkCoeff_ = 0.0;
    double relCoeff_ = 0.0;
    double vb_ = -1.5;
    double a0_ = 1.0;
    double driveLin_ = 1.0;

    // ADAA state
    double vgPrev_ = -1.5;
    double xPrev_ = 0.0;
    bool hasVgPrev_ = false;
    bool hasXPrev_ = false;
};

} // namespace wg::dsp
