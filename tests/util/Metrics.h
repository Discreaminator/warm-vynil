#pragma once

#include "Spectrum.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace wg::test {

[[nodiscard]] inline double lin2dB(double x) noexcept
{
    return 20.0 * std::log10(std::max(x, 1.0e-300));
}

[[nodiscard]] inline double binEnergy(const std::vector<double>& mag, int centre, int radius = 3) noexcept
{
    double e = 0.0;
    const int n = static_cast<int>(mag.size());
    for (int i = centre - radius; i <= centre + radius; ++i)
    {
        if (i >= 0 && i < n)
            e += mag[static_cast<std::size_t>(i)] * mag[static_cast<std::size_t>(i)];
    }
    return std::sqrt(e);
}

[[nodiscard]] inline double measureThd(const std::vector<double>& mag, double f0, double fs)
{
    const int fftSize = static_cast<int>((mag.size() - 1) * 2);
    const double h1 = binEnergy(mag, freqToBin(f0, fs, fftSize));
    if (h1 < 1.0e-30)
        return 0.0;
    double sum = 0.0;
    for (int k = 2; k <= 10; ++k)
    {
        const double fk = f0 * static_cast<double>(k);
        if (fk >= 20000.0 || fk >= fs * 0.5)
            break;
        sum += std::pow(binEnergy(mag, freqToBin(fk, fs, fftSize)), 2.0);
    }
    return std::sqrt(sum) / h1;
}

[[nodiscard]] inline double harmonicDb(const std::vector<double>& mag, double f0, int harm, double fs)
{
    const int fftSize = static_cast<int>((mag.size() - 1) * 2);
    const double h1 = binEnergy(mag, freqToBin(f0, fs, fftSize));
    const double hk = binEnergy(mag, freqToBin(f0 * static_cast<double>(harm), fs, fftSize));
    return lin2dB(hk / std::max(h1, 1.0e-300));
}

[[nodiscard]] inline double measureAliasing(const std::vector<double>& mag, double f0, double fs)
{
    const int fftSize = static_cast<int>((mag.size() - 1) * 2);
    const double h1 = binEnergy(mag, freqToBin(f0, fs, fftSize));
    const int n = static_cast<int>(mag.size());
    const int b20 = freqToBin(20.0, fs, fftSize);
    const int b20k = freqToBin(std::min(20000.0, fs * 0.5 - 1.0), fs, fftSize);
    double peak = 0.0;
    for (int b = std::max(b20, 1); b <= b20k && b < n; ++b)
    {
        bool near = false;
        for (int k = 1; k * f0 < fs * 0.5; ++k)
        {
            if (std::abs(b - freqToBin(f0 * static_cast<double>(k), fs, fftSize)) <= 5)
            {
                near = true;
                break;
            }
        }
        if (!near)
            peak = std::max(peak, mag[static_cast<std::size_t>(b)]);
    }
    return lin2dB(peak / std::max(h1, 1.0e-300));
}

} // namespace wg::test
