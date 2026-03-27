#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"

class OpticalPanel : public juce::Component
{
public:
    OpticalPanel();
    void resized() override;
    void paint (juce::Graphics& g) override;

    juce::Slider thresholdKnob;
    juce::Slider gainKnob;
    juce::Slider scHpfKnob;
    juce::ToggleButton bypassToggle;

private:
    juce::Label titleLabel;
    juce::Label threshLabel, gainLabel, hpfLabel, bypassLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpticalPanel)
};
