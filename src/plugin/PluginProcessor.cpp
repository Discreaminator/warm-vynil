#include "PluginProcessor.h"
#include "ProductInfo.h"
#include "ui/PluginEditor.h"

namespace wg::plugin {

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

PluginProcessor::~PluginProcessor() = default;

const juce::String PluginProcessor::getName() const
{
    return product::kName;
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    wg::dsp::Spec spec;
    spec.sampleRate = sampleRate;
    spec.maxBlock = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    engine_.prepare(spec);
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

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midi);

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

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

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

} // namespace wg::plugin

// JUCE plugin entry points
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wg::plugin::PluginProcessor();
}
