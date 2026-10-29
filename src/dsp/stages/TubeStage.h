#pragma once

#include "TubeLut.h"
#include "dsp/EngineParams.h"

namespace wg::dsp {

struct TubeTargets
{
    double driveDb = 0.0;
    double tubeBias = 0.0;
    double kDyn = 0.0;
    TubeModel model = TubeModel::Koren;
};

/** 12AX7 stage: Koren LUT or fast tanh (SPEC §4.5). No ADAA in M1. */
class TubeStage
{
public:
    static constexpr double kVsw = 1.5;

    void prepare(double fsi, TubeLut& lut) noexcept;

    void reset() noexcept { env_ = 0.0; }

    void setTargets(const TubeTargets& t) noexcept { targets_ = t; }

    [[nodiscard]] double processSample(double x) noexcept;

private:
    [[nodiscard]] double processKoren(double x) noexcept;

    [[nodiscard]] double processFast(double x) noexcept;

    void updateEnvCoeffs() noexcept;

    double fsi_ = 48000.0;
    TubeLut* lut_ = nullptr;
    TubeTargets targets_{};

    double env_ = 0.0;
    double attackCoeff_ = 0.0;
    double releaseCoeff_ = 0.0;
};

} // namespace wg::dsp
