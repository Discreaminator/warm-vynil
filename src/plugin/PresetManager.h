#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace wg::plugin {

/** Factory programs (host menu) + user .wgpreset API (UI later) — SPEC §9.3. */
class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts);

    [[nodiscard]] int getNumPrograms() const noexcept;
    [[nodiscard]] int getCurrentProgram() const noexcept { return currentProgram_; }
    void setCurrentProgram(int index);
    [[nodiscard]] juce::String getProgramName(int index) const;

    [[nodiscard]] juce::String getLastPresetName() const { return lastPresetName_; }
    void setLastPresetName(juce::String name) { lastPresetName_ = std::move(name); }

    /** User presets directory: Documents/WARM VYNIL/Presets */
    [[nodiscard]] static juce::File getUserPresetsDirectory();

    bool saveUserPreset(const juce::String& name);
    bool loadUserPreset(const juce::File& file);
    bool deleteUserPreset(const juce::File& file);
    [[nodiscard]] juce::Array<juce::File> listUserPresets() const;

    /** Apply factory preset by index without changing currentProgram_ (internal). */
    void applyFactoryPreset(int index);

private:
    juce::AudioProcessorValueTreeState& apvts_;
    int currentProgram_ = 0;
    juce::String lastPresetName_{ "По умолчанию" };

    void setParam(const char* id, float value);
    void setChoice(const char* id, int index);
    void setBool(const char* id, bool value);
};

} // namespace wg::plugin
