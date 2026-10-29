#pragma once

#include "ParamBridge.h"
#include "ParameterLayout.h"
#include "PresetManager.h"
#include "TransportState.h"
#include "dsp/Engine.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

namespace wg::plugin {

class PluginProcessor final : public juce::AudioProcessor,
                              private juce::AudioProcessorValueTreeState::Listener
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts_; }
    const juce::AudioProcessorValueTreeState& getAPVTS() const noexcept { return apvts_; }
    PresetManager& getPresetManager() noexcept { return presets_; }
    TransportState& getTransportState() noexcept { return transport_; }
    wg::dsp::Engine& getEngine() noexcept { return engine_; }

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void applyEngineTargets(bool allowStructuralPrepare);

    juce::AudioProcessorValueTreeState apvts_;
    ParamBridge bridge_;
    PresetManager presets_;
    TransportState transport_;
    wg::dsp::Engine engine_;

    double preparedSampleRate_ = 0.0;
    int preparedMaxBlock_ = 0;
    std::atomic<bool> structuralPending_{ false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};

} // namespace wg::plugin
