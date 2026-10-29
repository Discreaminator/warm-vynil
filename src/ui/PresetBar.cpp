#include "PresetBar.h"
#include "Fonts.h"
#include "PluginEditor.h"
#include "Strings.h"
#include "Theme.h"
#include "UiHelpers.h"
#include "plugin/ParameterIds.h"
#include "plugin/PluginProcessor.h"
#include "plugin/ProductInfo.h"

namespace wg::ui {

PresetBar::PresetBar(wg::plugin::PluginProcessor& processor)
    : processor_(processor)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    refreshName();
    processor_.getAPVTS().addParameterListener(wg::plugin::param::bypass, this);
    if (auto* p = processor_.getAPVTS().getParameter(wg::plugin::param::bypass))
        bypassOn_ = p->getValue() >= 0.5f;
}

PresetBar::~PresetBar()
{
    processor_.getAPVTS().removeParameterListener(wg::plugin::param::bypass, this);
}

void PresetBar::parameterChanged(const juce::String& id, float value)
{
    if (id == wg::plugin::param::bypass)
    {
        bypassOn_ = value >= 0.5f;
        repaint();
    }
}

void PresetBar::refreshName()
{
    presetName_ = processor_.getPresetManager().getLastPresetName();
    if (presetName_.isEmpty())
        presetName_ = processor_.getProgramName(processor_.getCurrentProgram());
    repaint();
}

void PresetBar::resized()
{
    prevBounds_ = { 196.0f, 12.0f, 30.0f, 24.0f };
    nextBounds_ = { 390.0f, 12.0f, 30.0f, 24.0f };
    nameBounds_ = { 226.0f, 12.0f, 164.0f, 24.0f };
    menuBounds_ = { 368.0f, 12.0f, 22.0f, 24.0f };
    saveBounds_ = { 428.0f, 12.0f, 24.0f, 24.0f };
    bypassBounds_ = { 462.0f, 12.0f, 84.0f, 24.0f };
}

void PresetBar::paint(juce::Graphics& g)
{
    auto& fonts = Fonts::get();

    // Logo (not dimmed with bypass — whole bar except bypass dims via parent layer; paint logo always full)
    {
        juce::Graphics::ScopedSaveState ss(g);
        if (dimmed_)
            g.beginTransparencyLayer(0.6f);

        g.setColour(theme::c(theme::accent));
        g.drawEllipse(24.0f - 7.0f, 24.0f - 7.0f, 14.0f, 14.0f, 1.6f);
        g.fillEllipse(24.0f - 2.0f, 24.0f - 2.0f, 4.0f, 4.0f);

        g.setFont(fonts.interTight(12.5f, 700));
        g.setColour(theme::c(theme::textPrimary));
        g.drawText(str::brand(), juce::Rectangle<float>(39.0f, 16.0f, 150.0f, 20.0f), juce::Justification::centredLeft, false);

        // Preset pill
        g.setColour(theme::c(theme::surface));
        g.fillRoundedRectangle(196.0f, 12.0f, 224.0f, 24.0f, 12.0f);
        g.setColour(theme::c(theme::line));
        g.drawRoundedRectangle(196.5f, 12.5f, 223.0f, 23.0f, 12.0f, 1.0f);

        drawChevronLeft(g, 211.5f, 24.0f, 8.0f, theme::c(theme::textSecondary));
        drawChevronRight(g, 404.5f, 24.0f, 8.0f, theme::c(theme::textSecondary));
        g.setColour(theme::c(theme::line));
        g.drawLine(226.0f, 17.0f, 226.0f, 31.0f, 1.0f);
        g.drawLine(390.0f, 17.0f, 390.0f, 31.0f, 1.0f);

        g.setFont(fonts.inter(12.0f, 500));
        g.setColour(theme::c(theme::textPrimary));
        g.drawText(presetName_, juce::Rectangle<float>(230.0f, 12.0f, 140.0f, 24.0f), juce::Justification::centred, true);

        drawChevronDown(g, 377.0f, 24.0f, 6.0f, theme::c(theme::textSecondary));

        // Save
        g.setColour(theme::c(theme::surface));
        g.fillRoundedRectangle(saveBounds_, 12.0f);
        g.setColour(theme::c(theme::line));
        g.drawRoundedRectangle(saveBounds_.reduced(0.5f), 12.0f, 1.0f);
        // Floppy icon
        juce::Path floppy;
        floppy.startNewSubPath(435.0f, 20.5f);
        floppy.lineTo(435.0f, 19.0f);
        floppy.lineTo(442.0f, 19.0f);
        floppy.lineTo(445.0f, 22.0f);
        floppy.lineTo(445.0f, 27.5f);
        floppy.lineTo(436.5f, 29.0f);
        floppy.lineTo(435.0f, 27.5f);
        floppy.closeSubPath();
        g.setColour(theme::c(theme::textSecondary));
        g.strokePath(floppy, juce::PathStrokeType(1.2f));

        if (dimmed_)
            g.endTransparencyLayer();
    }

    // Bypass — always full opacity
    g.setColour(theme::c(theme::surface));
    g.fillRoundedRectangle(bypassBounds_, 12.0f);
    if (bypassOn_)
        g.setColour(theme::c(theme::accent).withAlpha(0.4f));
    else
        g.setColour(theme::c(theme::line));
    g.drawRoundedRectangle(bypassBounds_.reduced(0.5f), 12.0f, 1.0f);

    const juce::Colour bypassCol = bypassOn_ ? theme::c(theme::accent) : theme::c(theme::textControl);
    // Power icon
    juce::Path power;
    power.addCentredArc(480.0f, 24.0f, 5.0f, 5.0f, 0.0f, juce::degreesToRadians(40.0f), juce::degreesToRadians(320.0f), true);
    g.setColour(bypassCol);
    g.strokePath(power, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.drawLine(480.0f, 17.8f, 480.0f, 23.25f, 1.5f);

    g.setFont(fonts.inter(12.0f, 500));
    g.setColour(bypassCol);
    g.drawText(str::bypass(), juce::Rectangle<float>(493.0f, 12.0f, 50.0f, 24.0f), juce::Justification::centredLeft, false);

    // Header divider
    if (dimmed_)
        g.beginTransparencyLayer(0.6f);
    g.setColour(theme::c(theme::divider));
    g.drawLine(0.0f, 48.5f, 560.0f, 48.5f, 1.0f);
    if (dimmed_)
        g.endTransparencyLayer();
}

void PresetBar::mouseUp(const juce::MouseEvent& e)
{
    const auto p = e.position;
    if (prevBounds_.contains(p))
    {
        prevPreset();
        return;
    }
    if (nextBounds_.contains(p))
    {
        nextPreset();
        return;
    }
    if (nameBounds_.contains(p) || menuBounds_.contains(p))
    {
        showPresetMenu();
        return;
    }
    if (saveBounds_.contains(p))
    {
        savePreset();
        return;
    }
    if (bypassBounds_.contains(p))
    {
        if (auto* bp = dynamic_cast<juce::AudioParameterBool*>(processor_.getAPVTS().getParameter(wg::plugin::param::bypass)))
        {
            const bool next = ! bp->get();
            bp->beginChangeGesture();
            bp->setValueNotifyingHost(next ? 1.0f : 0.0f);
            bp->endChangeGesture();
        }
    }
}

void PresetBar::prevPreset()
{
    auto& pm = processor_.getPresetManager();
    const int n = processor_.getNumPrograms();
    if (n <= 0)
        return;
    int i = processor_.getCurrentProgram() - 1;
    if (i < 0)
        i = n - 1;
    processor_.setCurrentProgram(i);
    pm.setLastPresetName(processor_.getProgramName(i));
    refreshName();
}

void PresetBar::nextPreset()
{
    auto& pm = processor_.getPresetManager();
    const int n = processor_.getNumPrograms();
    if (n <= 0)
        return;
    int i = (processor_.getCurrentProgram() + 1) % n;
    processor_.setCurrentProgram(i);
    pm.setLastPresetName(processor_.getProgramName(i));
    refreshName();
}

void PresetBar::requestScale(float scale)
{
    if (auto* ed = findParentComponentOfClass<PluginEditor>())
        ed->setUiScale(scale);
}

void PresetBar::savePreset()
{
    auto dir = wg::plugin::PresetManager::getUserPresetsDirectory();
    dir.createDirectory();

    auto* aw = new juce::AlertWindow(str::saveAs(),
                                     juce::CharPointer_UTF8("\xd0\x98\xd0\xbc\xd1\x8f \xd0\xbf\xd1\x80\xd0\xb5\xd1\x81\xd0\xb5\xd1\x82\xd0\xb0"),
                                     juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", presetName_);
    aw->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    aw->enterModalState(true, juce::ModalCallbackFunction::create([this, aw](int result) {
        std::unique_ptr<juce::AlertWindow> deleter(aw);
        if (result != 1)
            return;
        const auto name = aw->getTextEditorContents("name").trim();
        auto& presets = processor_.getPresetManager();
        if (name.isNotEmpty() && presets.saveUserPreset(name))
        {
            presets.setLastPresetName(name);
            refreshName();
        }
    }));
}

void PresetBar::showPresetMenu()
{
    juce::PopupMenu menu;
    juce::PopupMenu factory;
    for (int i = 0; i < processor_.getNumPrograms(); ++i)
        factory.addItem(100 + i, processor_.getProgramName(i), true, i == processor_.getCurrentProgram());
    menu.addSubMenu(str::factoryGroup(), factory);

    juce::PopupMenu user;
    auto files = processor_.getPresetManager().listUserPresets();
    for (int i = 0; i < files.size(); ++i)
        user.addItem(200 + i, files[i].getFileNameWithoutExtension());
    if (files.isEmpty())
        user.addItem(-1, "—", false);
    menu.addSubMenu(str::userGroup(), user);
    menu.addSeparator();
    menu.addItem(1, str::saveAs());
    menu.addItem(2, str::removePreset(), files.size() > 0);
    menu.addSeparator();
    juce::PopupMenu scale;
    const float scales[] = { 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
    for (int i = 0; i < 5; ++i)
        scale.addItem(300 + i, juce::String(juce::roundToInt(scales[i] * 100)) + " %");
    menu.addSubMenu(str::scaleMenu(), scale);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                       [this, files](int result) {
                           if (result == 0)
                               return;
                           if (result == 1)
                           {
                               savePreset();
                               return;
                           }
                           if (result == 2)
                           {
                               // delete current user file if matching name
                               for (auto& f : files)
                               {
                                   if (f.getFileNameWithoutExtension() == presetName_)
                                   {
                                       processor_.getPresetManager().deleteUserPreset(f);
                                       break;
                                   }
                               }
                               return;
                           }
                           if (result >= 100 && result < 200)
                           {
                               const int i = result - 100;
                               processor_.setCurrentProgram(i);
                               processor_.getPresetManager().setLastPresetName(processor_.getProgramName(i));
                               refreshName();
                               return;
                           }
                           if (result >= 200 && result < 300)
                           {
                               const int i = result - 200;
                               if (juce::isPositiveAndBelow(i, files.size()))
                               {
                                   processor_.getPresetManager().loadUserPreset(files[i]);
                                   processor_.getPresetManager().setLastPresetName(files[i].getFileNameWithoutExtension());
                                   refreshName();
                               }
                               return;
                           }
                           if (result >= 300 && result < 310)
                           {
                               const float scales[] = { 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
                               requestScale(scales[result - 300]);
                           }
                       });
}

} // namespace wg::ui
