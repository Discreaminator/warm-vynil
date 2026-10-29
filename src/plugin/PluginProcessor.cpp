#include "PluginProcessor.h"
#include "ParameterIds.h"
#include "ProductInfo.h"
#include "StateSerializer.h"
#include "ui/PluginEditor.h"

namespace wg::plugin {

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts_(*this, nullptr, "PARAMETERS", createParameterLayout())
    , bridge_(apvts_)
    , presets_(apvts_)
{
    apvts_.addParameterListener(param::osFactor, this);
    apvts_.addParameterListener(param::offlineHQ, this);
    apvts_.addParameterListener(param::adaa, this);
}

PluginProcessor::~PluginProcessor()
{
    apvts_.removeParameterListener(param::osFactor, this);
    apvts_.removeParameterListener(param::offlineHQ, this);
    apvts_.removeParameterListener(param::adaa, this);
}

const juce::String PluginProcessor::getName() const
{
    return product::kName;
}

void PluginProcessor::prepareEngine(const wg::dsp::EngineParams& targets)
{
    engine_.setTargets(targets);

    wg::dsp::Spec spec;
    spec.sampleRate = preparedSampleRate_;
    spec.maxBlock = preparedMaxBlock_;
    spec.numChannels = getTotalNumOutputChannels();
    spec.nonRealtime = hostNonRealtime_;
    engine_.prepare(spec);
    setLatencySamples(engine_.latencySamples());
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    preparedSampleRate_ = sampleRate;
    preparedMaxBlock_ = samplesPerBlock;
    hostNonRealtime_ = isNonRealtime();

    auto targets = bridge_.snapshot();
    prepareEngine(targets);

    bridge_.noteStructuralApplied(targets);
    structuralPending_.store(false, std::memory_order_release);
}

void PluginProcessor::releaseResources()
{
    engine_.reset();
}

void PluginProcessor::setNonRealtime(bool isNonRealtimeFlag) noexcept
{
    AudioProcessor::setNonRealtime(isNonRealtimeFlag);
    hostNonRealtime_ = isNonRealtimeFlag;

    if (preparedSampleRate_ <= 0.0)
        return;

    // offlineHQ toggles effective OS / ADAA on realtime↔offline transitions.
    auto targets = bridge_.snapshot();
    if (!targets.offlineHQ)
    {
        engine_.setNonRealtime(isNonRealtimeFlag);
        return;
    }

    suspendProcessing(true);
    prepareEngine(targets);
    bridge_.noteStructuralApplied(targets);
    structuralPending_.store(false, std::memory_order_release);
    suspendProcessing(false);
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void PluginProcessor::applyEngineTargets(bool allowStructuralPrepare)
{
    auto targets = bridge_.snapshot();

    // Effective OS can also change when offlineHQ + host nonRealtime flips.
    const int wantOs = (targets.offlineHQ && hostNonRealtime_) ? 8 : targets.osFactor;
    const bool wantAdaa = targets.adaa || (targets.offlineHQ && hostNonRealtime_);
    const bool topologyDrift = bridge_.structuralChanged(targets)
                               || wantOs != engine_.preparedOsChoice()
                               || wantAdaa != engine_.adaaActive();

    if (topologyDrift)
    {
        if (!allowStructuralPrepare)
        {
            // Keep prior OS on the audio thread; message thread will re-prepare.
            targets.osFactor = engine_.preparedOsChoice();
            targets.adaa = engine_.adaaActive();
            engine_.setTargets(targets);
            structuralPending_.store(true, std::memory_order_release);
            return;
        }

        prepareEngine(targets);
        bridge_.noteStructuralApplied(targets);
        structuralPending_.store(false, std::memory_order_release);
        return;
    }

    engine_.setTargets(targets);
}

void PluginProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused(parameterID, newValue);
    // Structural params: re-prepare off the audio thread when possible.
    if (preparedSampleRate_ <= 0.0)
        return;

    suspendProcessing(true);
    applyEngineTargets(true);
    suspendProcessing(false);
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midi);

    // Transport for M3 vinyl spin (SPEC §9.2).
    bool playing = false;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
            playing = pos->getIsPlaying();
    }
    transport_.isPlaying.store(playing, std::memory_order_relaxed);

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    applyEngineTargets(false);

    float* channels[2] = {
        buffer.getNumChannels() > 0 ? buffer.getWritePointer(0) : nullptr,
        buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr,
    };

    engine_.process(channels, buffer.getNumChannels(), buffer.getNumSamples());
}

void PluginProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    // Keep PDC constant: delayed dry via Engine without touching the bypass param.
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midi);

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto targets = bridge_.snapshot();
    targets.bypass = true;
    // Do not allow structural prepare on the audio thread.
    targets.osFactor = engine_.preparedOsChoice();
    targets.adaa = engine_.adaaActive();
    engine_.setTargets(targets);

    float* channels[2] = {
        buffer.getNumChannels() > 0 ? buffer.getWritePointer(0) : nullptr,
        buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr,
    };
    engine_.process(channels, buffer.getNumChannels(), buffer.getNumSamples());
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new wg::ui::PluginEditor(*this);
}

int PluginProcessor::getNumPrograms()
{
    return presets_.getNumPrograms();
}

int PluginProcessor::getCurrentProgram()
{
    return presets_.getCurrentProgram();
}

void PluginProcessor::setCurrentProgram(int index)
{
    presets_.setCurrentProgram(index);
}

const juce::String PluginProcessor::getProgramName(int index)
{
    return presets_.getProgramName(index);
}

void PluginProcessor::changeProgramName(int, const juce::String&)
{
    // Factory programs are read-only.
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    StateSerializer::getState(apvts_, destData, presets_.getLastPresetName());
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    StateSerializer::setState(apvts_, data, sizeInBytes);
    bridge_.bind(apvts_);

    if (auto state = apvts_.copyState(); state.hasProperty("lastPresetName"))
        presets_.setLastPresetName(state.getProperty("lastPresetName").toString());

    if (preparedSampleRate_ > 0.0)
    {
        suspendProcessing(true);
        applyEngineTargets(true);
        suspendProcessing(false);
    }
}

juce::AudioProcessorParameter* PluginProcessor::getBypassParameter() const
{
    return apvts_.getParameter(param::bypass);
}

} // namespace wg::plugin
