#pragma once

#include "EngineParams.h"
#include "MacroMap.h"
#include "Smoothing.h"
#include "filters/DcBlocker.h"
#include "os/Oversampler.h"
#include "stages/AutoGain.h"
#include "stages/Cartridge.h"
#include "stages/Crosstalk.h"
#include "stages/MonoBass.h"
#include "stages/Riaa.h"
#include "stages/Tracing.h"
#include "stages/Transformer.h"
#include "stages/TubeStage.h"
#include "util/DelayLine.h"
#include "util/Denormals.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace wg::dsp {

struct Spec
{
    double sampleRate = 48000.0;
    int maxBlock = 512;
    int numChannels = 2;
    /** Host non-realtime (offline bounce). Combined with offlineHQ → 8x+ADAA. */
    bool nonRealtime = false;
};

/** Full DSP chain (SPEC §3–§6). M4: ADAA, OS8, offlineHQ constant latency. */
class Engine
{
public:
    void prepare(const Spec& spec) noexcept;
    void reset() noexcept;
    void setTargets(const EngineParams& params) noexcept;

    /** Notify realtime ↔ offline transitions without a full structural prepare. */
    void setNonRealtime(bool nonRealtime) noexcept;

    /** In-place process. channels[c][n], numChannels = 1 or 2. */
    void process(float* const* channels, int numChannels, int numSamples) noexcept;

    [[nodiscard]] int latencySamples() const noexcept { return latencySamples_; }
    [[nodiscard]] const Spec& spec() const noexcept { return spec_; }
    [[nodiscard]] int osFactor() const noexcept { return oversampler_.factor(); }
    [[nodiscard]] int preparedOsChoice() const noexcept { return preparedOsChoice_; }
    [[nodiscard]] bool adaaActive() const noexcept { return adaaActive_; }
    [[nodiscard]] double internalRate() const noexcept { return oversampler_.internalRate(); }

    /** Expose stages for isolated tests. */
    [[nodiscard]] Riaa& riaa(int ch) noexcept { return riaa_[static_cast<std::size_t>(ch)]; }
    [[nodiscard]] Tracing& tracing(int ch) noexcept { return tracing_[static_cast<std::size_t>(ch)]; }
    [[nodiscard]] os::Oversampler& oversampler() noexcept { return oversampler_; }
    [[nodiscard]] TubeStage& tube(int ch) noexcept { return tube_[static_cast<std::size_t>(ch)]; }

private:
    void updateSmoothersAndCoeffs() noexcept;
    void processOsChain(float* leftOs, float* rightOs, int numOs, int numChannels) noexcept;
    [[nodiscard]] int resolveOsChoice() const noexcept;
    [[nodiscard]] bool resolveAdaa() const noexcept;

    Spec spec_{};
    EngineParams targets_{};
    int latencySamples_ = 0;
    int wetPadSamples_ = 0;
    int preparedOsChoice_ = 4;
    bool adaaActive_ = true;
    std::uint64_t sampleCounter_ = 0;

    LinearSmoother amountSm_{};
    DbSmoother inGainSm_{};
    DbSmoother outGainSm_{};
    LinearSmoother mixSm_{};
    LinearSmoother wetEnableSm_{}; // engine-bypass wet weight
    LinearSmoother hostBypassSm_{};
    LinearSmoother structuralFadeSm_{}; // 10 ms fade after OS/topology prepare

    DcBlocker<double> dcIn_[2]{};
    DcBlocker<double> dcOut_[2]{};
    MonoBass monoBass_{};
    os::Oversampler oversampler_{};

    std::array<Riaa, 2> riaa_{};
    std::array<Tracing, 2> tracing_{};
    std::array<Cartridge, 2> cartridge_{};
    Crosstalk crosstalk_{};
    std::array<TubeStage, 2> tube_{};
    std::array<Transformer, 2> xfmr_{};
    AutoGain autoGain_{};
    double autoGainLin_ = 1.0;

    DelayLine dryDelayL_{};
    DelayLine dryDelayR_{};
    DelayLine wetPadL_{};
    DelayLine wetPadR_{};

    std::vector<float> osL_{};
    std::vector<float> osR_{};
    std::vector<float> dryL_{};
    std::vector<float> dryR_{};

    bool engineWetActive_ = true;
    InternalTargets internal_{};
};

} // namespace wg::dsp
