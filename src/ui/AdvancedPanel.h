#pragma once

#include "RotaryKnob.h"
#include "SegmentedControl.h"
#include "ToggleSwitch.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace wg::plugin {
class PluginProcessor;
}

namespace wg::ui {

class AdvancedPanel final : public juce::Component
{
public:
    explicit AdvancedPanel(wg::plugin::PluginProcessor& processor);
    ~AdvancedPanel() override = default;

    void setDimmed(bool dim);
    void setPlaying(bool playing);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    struct Card
    {
        juce::Rectangle<float> bounds;
        juce::String title;
    };

    wg::plugin::PluginProcessor& processor_;
    bool dimmed_ = false;

    std::vector<std::unique_ptr<RotaryKnob>> knobs_;
    std::unique_ptr<ToggleSwitch> neumann_;
    std::unique_ptr<ToggleSwitch> adaa_;
    std::unique_ptr<ToggleSwitch> offlineHQ_;
    std::unique_ptr<SegmentedControl> osFactor_;
    std::unique_ptr<juce::Component> tubeModel_; // painted chips; click handled here
    int tubeModelIndex_ = 0;
    std::unique_ptr<juce::ParameterAttachment> tubeAttach_;

    Card cards_[7]{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AdvancedPanel)
};

} // namespace wg::ui
