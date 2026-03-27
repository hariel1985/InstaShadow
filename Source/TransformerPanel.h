#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"

class TransformerPanel : public juce::Component
{
public:
    TransformerPanel();
    void resized() override;
    void paint (juce::Graphics& g) override;

    int getSelectedType() const { return currentType; }
    void setSelectedType (int type);

    juce::TextButton nickelButton  { "NICKEL" };
    juce::TextButton ironButton    { "IRON" };
    juce::TextButton steelButton   { "STEEL" };

private:
    juce::Label titleLabel;
    int currentType = 0;  // 0=Off

    void updateButtonStates();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransformerPanel)
};
