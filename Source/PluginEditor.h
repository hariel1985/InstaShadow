#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "OpticalPanel.h"
#include "DiscretePanel.h"
#include "TransformerPanel.h"
#include "OutputPanel.h"
#include "GRMeter.h"
#include "NeedleVuMeter.h"

static constexpr const char* kInstaShadowVersion = "v1.1";

class InstaShadowEditor : public juce::AudioProcessorEditor,
                           public juce::Timer
{
public:
    explicit InstaShadowEditor (InstaShadowProcessor&);
    ~InstaShadowEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    InstaShadowProcessor& processor;
    InstaShadowLookAndFeel lookAndFeel;

    // Top bar
    juce::Label titleLabel;
    juce::Label versionLabel;
    juce::ToggleButton linkToggle;
    juce::Label linkLabel;
    juce::ToggleButton bypassToggle;
    juce::Label bypassLabel;

    // Side panels
    OpticalPanel opticalPanel;
    DiscretePanel discretePanel;

    // Center: needle meters + bar meters (swappable)
    NeedleVuMeter needleMeterL;
    NeedleVuMeter needleMeterR;
    GRMeter barMeterL;
    GRMeter barMeterR;

    // Meter swap toggle
    juce::TextButton meterSwapButton { "GR / INPUT" };
    bool metersSwapped = false;  // false: needle=GR, bar=input | true: needle=input, bar=GR

    // Bottom panels
    TransformerPanel transformerPanel;
    OutputPanel outputPanel;

    // The controls are bound to the host parameters; the editor no longer
    // writes its own values into the engine.
    std::vector<std::unique_ptr<juce::SliderParameterAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<juce::ButtonParameterAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<juce::ComboBoxParameterAttachment>> comboAttachments;
    std::unique_ptr<juce::ParameterAttachment> transformerAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstaShadowEditor)
};
