#pragma once

#include <array>
#include <cstddef>

namespace wg::dsp {

/** Immutable shared 12AX7 plate-voltage LUT + antiderivative (SPEC §4.5). Built in prepare. */
class TubeLut
{
public:
    static constexpr std::size_t kNumPoints = 4097;
    static constexpr double kVgkMin = -6.0;
    static constexpr double kVgkMax = 2.0;

    void build() noexcept;

    [[nodiscard]] bool ready() const noexcept { return ready_; }

    /** Hermite interpolate Vp(Vgk). */
    [[nodiscard]] double lookupVp(double vgk) const noexcept;

    /** Slope dVp/dVgk at Vgk (from LUT differences). */
    [[nodiscard]] double lookupSlope(double vgk) const noexcept;

    /** Antiderivative P(Vgk) for ADAA (M4). */
    [[nodiscard]] double lookupP(double vgk) const noexcept;

private:
    [[nodiscard]] static double korenIp(double vgk, double vp) noexcept;
    [[nodiscard]] static double solveVp(double vgk) noexcept;

    std::array<double, kNumPoints> vp_{};
    std::array<double, kNumPoints> p_{};
    bool ready_ = false;
};

} // namespace wg::dsp
