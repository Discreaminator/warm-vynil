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

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    preparedSampleRate_ = sampleRate;
    preparedMaxBlock_ = samplesPerBlock;

    auto targets = bridge_.snapshot();
    // offlineHQ may bump OS in M4; M2 stores the flag only.
    engine_.setTargets(targets);

    wg::dsp::Spec spec;
    spec.sampleRate = sampleRate;
    spec.maxBlock = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    engine_.prepare(spec);

    bridge_.noteStructuralApplied(targets);
    structuralPending_.store(false, std::memory_order_release);
    setLatencySamples(engine_.latencySamples());
}

void PluginProcessor::releaseResources()
{
    engine_.reset();
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

    if (bridge_.structuralChanged(targets))
    {
        if (!allowStructuralPrepare)
        {
            // Keep prior OS on the audio thread; message thread will re-prepare.
            targets.osFactor = engine_.osFactor();
            engine_.setTargets(targets);
            structuralPending_.store(true, std::memory_order_release);
            return;
        }

        engine_.setTargets(targets);
        if (preparedSampleRate_ > 0.0)
        {
            wg::dsp::Spec spec;
            spec.sampleRate = preparedSampleRate_;
            spec.maxBlock = preparedMaxBlock_;
            spec.numChannels = getTotalNumOutputChannels();
            engine_.prepare(spec);
            setLatencySamples(engine_.latencySamples());
        }
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
