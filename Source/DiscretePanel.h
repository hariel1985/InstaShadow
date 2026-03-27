#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"

class DiscretePanel : public juce::Component
{
public:
    DiscretePanel();
    void resized() override;
    void paint (juce::Graphics& g) override;

    juce::Slider thresholdKnob;
    juce::Slider gainKnob;
    juce::ComboBox ratioBox;
    juce::ComboBox attackBox;
    juce::ComboBox releaseBox;
    juce::ToggleButton bypassToggle;

private:
    juce::Label titleLabel;
    juce::Label threshLabel, gainLabel, ratioLabel, attackLabel, releaseLabel, bypassLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DiscretePanel)
};
