#pragma once

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127 4324)
#endif
#include <pocketfft_hdronly.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "SignalGen.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

namespace wg::test {

inline void blackmanHarris7(std::vector<double>& w)
{
    const int N = static_cast<int>(w.size());
    constexpr double a0 = 0.27105140069342;
    constexpr double a1 = 0.43329793923448;
    constexpr double a2 = 0.21812299954311;
    constexpr double a3 = 0.06592544638803;
    constexpr double a4 = 0.01081174209837;
    constexpr double a5 = 0.00077658482522;
    constexpr double a6 = 0.00001388721735;
    for (int n = 0; n < N; ++n)
    {
        const double p = 2.0 * kPi * static_cast<double>(n) / static_cast<double>(N - 1);
        w[static_cast<std::size_t>(n)] = a0 - a1 * std::cos(p) + a2 * std::cos(2 * p) - a3 * std::cos(3 * p)
                                         + a4 * std::cos(4 * p) - a5 * std::cos(5 * p) + a6 * std::cos(6 * p);
    }
}

[[nodiscard]] inline int freqToBin(double f, double fs, int fftSize = kFftSize) noexcept
{
    return static_cast<int>(std::round(f * static_cast<double>(fftSize) / fs));
}

/** Magnitude of last min(nSamples, fftSize) samples. */
[[nodiscard]] inline std::vector<double> magnitudeSpectrum(const float* x, int nSamples, int fftSize = kFftSize)
{
    std::vector<std::complex<double>> td(static_cast<std::size_t>(fftSize), { 0.0, 0.0 });
    std::vector<double> win(static_cast<std::size_t>(fftSize), 1.0);
    blackmanHarris7(win);
    double wsum = 0.0;
    for (double v : win)
        wsum += v;

    const int use = std::min(nSamples, fftSize);
    const int start = nSamples - use;
    for (int i = 0; i < use; ++i)
        td[static_cast<std::size_t>(i)] = { static_cast<double>(x[start + i]) * win[static_cast<std::size_t>(i)], 0.0 };

    pocketfft::shape_t shape{ static_cast<std::size_t>(fftSize) };
    pocketfft::stride_t st{ static_cast<std::ptrdiff_t>(sizeof(std::complex<double>)) };
    pocketfft::shape_t axes{ 0 };
    pocketfft::c2c(shape, st, st, axes, pocketfft::FORWARD, td.data(), td.data(), 1.0);

    const double norm = (wsum > 0.0) ? (2.0 / wsum) : 1.0;
    std::vector<double> mag(static_cast<std::size_t>(fftSize / 2 + 1));
    for (int i = 0; i <= fftSize / 2; ++i)
        mag[static_cast<std::size_t>(i)] = std::abs(td[static_cast<std::size_t>(i)]) * norm;
    mag[0] *= 0.5;
    return mag;
}

} // namespace wg::test
