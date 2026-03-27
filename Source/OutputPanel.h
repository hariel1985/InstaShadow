#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "VuMeter.h"

class OutputPanel : public juce::Component
{
public:
    OutputPanel();
    void resized() override;
    void paint (juce::Graphics& g) override;

    juce::Slider outputGainKnob;
    VuMeter vuMeter;

private:
    juce::Label titleLabel;
    juce::Label gainLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputPanel)
};
