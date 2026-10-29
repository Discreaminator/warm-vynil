#include "ParamBridge.h"
#include "ParameterIds.h"

#include <cmath>

namespace wg::plugin {
namespace {

float load(std::atomic<float>* p, float fallback) noexcept
{
    return p != nullptr ? p->load(std::memory_order_relaxed) : fallback;
}

int choiceIndex(std::atomic<float>* p, int fallback, int maxInclusive) noexcept
{
    const int v = static_cast<int>(std::lround(static_cast<double>(load(p, static_cast<float>(fallback)))));
    return juce::jlimit(0, maxInclusive, v);
}

int osFactorFromChoice(int index) noexcept
{
    switch (index)
    {
    case 0:
        return 2;
    case 2:
        return 8;
    default:
        return 4;
    }
}

} // namespace

ParamBridge::ParamBridge(juce::AudioProcessorValueTreeState& apvts)
{
    bind(apvts);
}

void ParamBridge::bind(juce::AudioProcessorValueTreeState& apvts)
{
    amount_ = apvts.getRawParameterValue(param::amount);
    outGain_ = apvts.getRawParameterValue(param::outGain);
    autoGain_ = apvts.getRawParameterValue(param::autoGain);
    bypass_ = apvts.getRawParameterValue(param::bypass);
    inGain_ = apvts.getRawParameterValue(param::inGain);
    mix_ = apvts.getRawParameterValue(param::mix);
    monoFreq_ = apvts.getRawParameterValue(param::monoFreq);
    monoDepth_ = apvts.getRawParameterValue(param::monoDepth);
    tracingTrim_ = apvts.getRawParameterValue(param::tracingTrim);
    riaaTilt_ = apvts.getRawParameterValue(param::riaaTilt);
    riaaNeumann_ = apvts.getRawParameterValue(param::riaaNeumann);
    cartFreq_ = apvts.getRawParameterValue(param::cartFreq);
    cartQ_ = apvts.getRawParameterValue(param::cartQ);
    crosstalk_ = apvts.getRawParameterValue(param::crosstalk);
    tubeTrim_ = apvts.getRawParameterValue(param::tubeTrim);
    tubeBias_ = apvts.getRawParameterValue(param::tubeBias);
    tubeModel_ = apvts.getRawParameterValue(param::tubeModel);
    xfmrSat_ = apvts.getRawParameterValue(param::xfmrSat);
    xfmrHF_ = apvts.getRawParameterValue(param::xfmrHF);
    osFactor_ = apvts.getRawParameterValue(param::osFactor);
    adaa_ = apvts.getRawParameterValue(param::adaa);
    offlineHQ_ = apvts.getRawParameterValue(param::offlineHQ);
}

wg::dsp::EngineParams ParamBridge::snapshot() const noexcept
{
    wg::dsp::EngineParams p{};

    // UI stores percent 0–100; Engine uses 0–1 (or 0–2 for trims).
    p.amount = load(amount_, 50.0f) * 0.01f;
    p.outGainDb = load(outGain_, 0.0f);
    p.autoGain = static_cast<wg::dsp::AutoGainMode>(choiceIndex(autoGain_, 1, 2));
    p.bypass = load(bypass_, 0.0f) >= 0.5f;
    p.inGainDb = load(inGain_, 0.0f);
    p.mix = load(mix_, 100.0f) * 0.01f;
    p.monoFreq = load(monoFreq_, 150.0f);
    p.monoDepth = load(monoDepth_, 100.0f) * 0.01f;
    p.tracingTrim = load(tracingTrim_, 100.0f) * 0.01f;
    p.riaaTilt = load(riaaTilt_, 0.5f);
    p.riaaNeumann = load(riaaNeumann_, 1.0f) >= 0.5f;
    p.cartFreq = load(cartFreq_, 14000.0f);
    p.cartQ = load(cartQ_, 1.2f);
    p.crosstalkDb = load(crosstalk_, -30.0f);
    p.tubeTrim = load(tubeTrim_, 100.0f) * 0.01f;
    p.tubeBias = load(tubeBias_, 0.0f);
    p.tubeModel = static_cast<wg::dsp::TubeModel>(choiceIndex(tubeModel_, 0, 1));
    p.xfmrSat = load(xfmrSat_, 40.0f) * 0.01f;
    p.xfmrHF = load(xfmrHF_, 20000.0f);
    p.osFactor = osFactorFromChoice(choiceIndex(osFactor_, 1, 2));
    p.adaa = load(adaa_, 1.0f) >= 0.5f;
    p.offlineHQ = load(offlineHQ_, 0.0f) >= 0.5f;
    return p;
}

bool ParamBridge::structuralChanged(const wg::dsp::EngineParams& next) const noexcept
{
    return next.osFactor != lastOsFactor_ || next.adaa != lastAdaa_ || next.offlineHQ != lastOfflineHQ_;
}

void ParamBridge::noteStructuralApplied(const wg::dsp::EngineParams& applied) noexcept
{
    lastOsFactor_ = applied.osFactor;
    lastAdaa_ = applied.adaa;
    lastOfflineHQ_ = applied.offlineHQ;
}

} // namespace wg::plugin
