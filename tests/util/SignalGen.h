#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace wg::test {

inline constexpr double kPi = 3.14159265358979323846;
inline constexpr int kFftSize = 65536;

[[nodiscard]] inline float dB2lin(double db) noexcept
{
    return static_cast<float>(std::pow(10.0, db / 20.0));
}

[[nodiscard]] inline double coherentFreq(double f, double fs, int nfft = kFftSize) noexcept
{
    const double bin = std::round(f * static_cast<double>(nfft) / fs);
    return bin * fs / static_cast<double>(nfft);
}

inline void fillSine(std::vector<float>& buf, double fs, double freq, double levelDb, double phase = 0.0)
{
    const float amp = dB2lin(levelDb);
    const double w = 2.0 * kPi * freq / fs;
    for (std::size_t i = 0; i < buf.size(); ++i)
        buf[i] = amp * static_cast<float>(std::sin(w * static_cast<double>(i) + phase));
}

[[nodiscard]] inline std::vector<float> sine(double freq, double fs, int n, double amp = 1.0, double phase = 0.0)
{
    std::vector<float> out(static_cast<std::size_t>(n));
    const double w = 2.0 * kPi * freq / fs;
    for (int i = 0; i < n; ++i)
        out[static_cast<std::size_t>(i)] = static_cast<float>(amp * std::sin(w * i + phase));
    return out;
}

[[nodiscard]] inline std::vector<float> impulse(int n, int at = 0)
{
    std::vector<float> out(static_cast<std::size_t>(n), 0.0f);
    if (at >= 0 && at < n)
        out[static_cast<std::size_t>(at)] = 1.0f;
    return out;
}

inline void fillPink(std::vector<float>& buf, double levelDb, std::uint32_t seed = 0xC0FFEEu)
{
    const float amp = dB2lin(levelDb);
    std::uint32_t s = seed ? seed : 1u;
    double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    for (float& sample : buf)
    {
        s = s * 1664525u + 1013904223u;
        const double white = (static_cast<double>(s >> 8) / 16777216.0) * 2.0 - 1.0;
        b0 = 0.99886 * b0 + white * 0.0555179;
        b1 = 0.99332 * b1 + white * 0.0750759;
        b2 = 0.96900 * b2 + white * 0.1538520;
        b3 = 0.86650 * b3 + white * 0.3104856;
        b4 = 0.55000 * b4 + white * 0.5329522;
        b5 = -0.7616 * b5 - white * 0.0168980;
        const double pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362;
        b6 = white * 0.115926;
        sample = amp * static_cast<float>(pink * 0.11);
    }
}

} // namespace wg::test
