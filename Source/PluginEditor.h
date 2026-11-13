#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Theme.h"
#include "Widgets.h"
#include "Meters.h"

// LookAndFeel for the parts JUCE draws itself (menus, tooltips, text entry).
class ThemeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ThemeLookAndFeel();
    void setTheme (ThemeId);
    ThemeId getTheme() const { return theme; }

    juce::Font getPopupMenuFont() override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
    void drawTooltip (juce::Graphics&, const juce::String&, int width, int height) override;

private:
    ThemeId theme = ThemeId::graphite;
};

// Inline "save preset" dialog (no separate window, which plugin hosts handle poorly).
class SavePresetPanel : public juce::Component
{
public:
    SavePresetPanel();

    void show (ThemeId, const juce::String& suggestedName);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;

    std::function<void (const juce::String&)> onSave;

private:
    ThemeId theme = ThemeId::graphite;
    juce::TextEditor nameEditor;
    juce::TextButton saveButton { "Save" }, cancelButton { "Cancel" };
    juce::Rectangle<int> box() const;
    void commit();
};

class InstaShadowEditor : public juce::AudioProcessorEditor,
                          private juce::Timer,
                          private juce::ChangeListener
{
public:
    explicit InstaShadowEditor (InstaShadowProcessor&);
    ~InstaShadowEditor() override;

    void resized() override;
    void paint (juce::Graphics&) override;

    void setTheme (ThemeId);
    ThemeId getTheme() const { return theme; }

    // ----- for tests -----
    RepaintCounter meterRepaints;
    int contentPaints = 0;              // Content::paint calls (copies of the cached background)
    int backgroundRenders = 0;          // times the static background was drawn
    int getTimerRate() const { return timerRate; }
    void stepMetersForTest() { timerCallback(); }
    juce::Component& getContent() { return content; }
    void setMeterMode (MeterMode);

    static constexpr int activeRate = 30;   // Hz while a meter moves (the ballistics are per tick)
    static constexpr int idleRate = 8;      // Hz when all meters are at rest: polling only, no repaints

private:
    InstaShadowProcessor& processor;
    juce::AudioProcessorValueTreeState& state;
    PresetManager& presets;
    ThemeLookAndFeel lookAndFeel;
    ThemeId theme = ThemeId::graphite;

    // All controls live in a fixed 1000 x 540 component scaled to the window.
    // The static background (panels, captions) is rendered once into an image
    // at the screen's pixel scale; a meter or knob repaint only copies from it.
    struct Content : public juce::Component
    {
        explicit Content (InstaShadowEditor& e) : owner (e) { setOpaque (true); }
        void paint (juce::Graphics&) override;
        void invalidateBackground() { background = {}; repaint(); }
        InstaShadowEditor& owner;
        juce::Image background;
        float backgroundScale = 0.0f;
    } content { *this };

    // Header
    HeaderButton prevButton { HeaderButton::Kind::previous };
    HeaderButton nameButton { HeaderButton::Kind::presetName };
    HeaderButton nextButton { HeaderButton::Kind::next };
    HeaderButton saveButton { HeaderButton::Kind::save };
    HeaderButton compareA   { HeaderButton::Kind::compareA };
    HeaderButton compareB   { HeaderButton::Kind::compareB };
    ThemeSwatch swatches[numThemes] { ThemeSwatch (ThemeId::graphite), ThemeSwatch (ThemeId::console), ThemeSwatch (ThemeId::ivory) };
    ThemedToggle linkToggle, bypassToggle;

    // Optical stage
    ThemedToggle optoIn;
    Knob optoThreshold, optoGain, optoHpf;
    ValueLabel optoThresholdValue { optoThreshold }, optoGainValue { optoGain }, optoHpfValue { optoHpf };
    NeedleMeter optoNeedle { "OPTICAL", "Optical" };
    SegmentMeter optoSegments { Console::cyan };

    // Discrete stage
    ThemedToggle vcaIn;
    Knob vcaThreshold, vcaGain;
    ValueLabel vcaThresholdValue { vcaThreshold }, vcaGainValue { vcaGain };
    ChoiceSelector ratioSelector, attackSelector, releaseSelector;
    NeedleMeter vcaNeedle { "DISCRETE", "Discrete" };
    SegmentMeter vcaSegments { Console::orange };

    // Transformer and output
    TransformerSelector transformer;
    Knob outGain;
    ValueLabel outGainValue { outGain };
    OutputMeter outMeter;

    std::vector<std::unique_ptr<juce::SliderParameterAttachment>> knobAttachments;

    SavePresetPanel savePanel;
    juce::TooltipWindow tooltipWindow { this, 700 };

    MeterMode meterMode = MeterMode::gainReduction;
    uint32_t lastMeterSequence = 0;
    int staleTicks = 0, idleTicks = 0;
    int timerRate = 0;
    bool constructed = false;

    void paintContent (juce::Graphics&);
    void paintGraphite (juce::Graphics&);
    void paintConsole (juce::Graphics&);
    void paintIvory (juce::Graphics&);

    void layoutGraphite();
    void layoutConsole();
    void layoutIvory();

    void setupKnob (Knob&, const char* paramID, std::function<juce::String (double)> formatter);
    void updateKnobFormatters();
    void updatePresetDisplay();
    void showPresetMenu();
    void showSavePanel();
    void toggleMeterMode();
    void resetMeters();
    void setTimerRate (int hz);

    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstaShadowEditor)
};
