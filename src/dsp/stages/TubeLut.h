#pragma once

#include <array>
#include <cstddef>

namespace wg::dsp {

/** Immutable 12AX7 Koren plate voltage LUT (SPEC §4.5). Built in prepare(), not audio thread. */
class TubeLut
{
public:
    static constexpr int kNumPoints = 4097;
    static constexpr double kVgkMin = -6.0;
    static constexpr double kVgkMax = 2.0;
    static constexpr double kVbias = -1.5;

    void prepare() noexcept;

    [[nodiscard]] bool isReady() const noexcept { return ready_; }

    [[nodiscard]] double evaluateVp(double vgk) const noexcept;

    /** dVp/dVgk from LUT (A0 at bias). */
    [[nodiscard]] double evaluateA0(double vgk) const noexcept;

    [[nodiscard]] double vpAtBias() const noexcept { return evaluateVp(kVbias); }

    [[nodiscard]] double a0AtBias() const noexcept { return evaluateA0(kVbias); }

private:
    [[nodiscard]] static double solveVp(double vgkEff) noexcept;

    std::array<double, kNumPoints> vp_{};
    std::array<double, kNumPoints> a0_{};
    bool ready_ = false;
};

} // namespace wg::dsp
