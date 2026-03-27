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

static constexpr const char* kInstaShadowVersion = "v1.0";

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

    // Center: needle VU meters + GR bars
    NeedleVuMeter vuMeterL;
    NeedleVuMeter vuMeterR;
    GRMeter optoGrMeter;
    GRMeter vcaGrMeter;

    // Bottom panels
    TransformerPanel transformerPanel;
    OutputPanel outputPanel;

    void syncKnobsToEngine();
    void syncEngineFromKnobs();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstaShadowEditor)
};
