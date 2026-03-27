#include "OutputPanel.h"

OutputPanel::OutputPanel()
{
    titleLabel.setText ("OUTPUT", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textPrimary);
    addAndMakeVisible (titleLabel);

    outputGainKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
    outputGainKnob.setRange (-12.0, 12.0);
    outputGainKnob.setValue (0.0);
    outputGainKnob.setTextValueSuffix (" dB");
    outputGainKnob.getProperties().set (InstaShadowLookAndFeel::knobTypeProperty, "orange");
    addAndMakeVisible (outputGainKnob);

    gainLabel.setText ("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType (juce::Justification::centred);
    gainLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textSecondary);
    addAndMakeVisible (gainLabel);

    addAndMakeVisible (vuMeter);
}

void OutputPanel::resized()
{
    auto bounds = getLocalBounds().reduced (4);
    titleLabel.setBounds (bounds.removeFromTop (20));

    int halfW = bounds.getWidth() / 2;

    auto knobArea = bounds.removeFromLeft (halfW);
    auto knobRect = knobArea.withTrimmedBottom (16);
    outputGainKnob.setBounds (knobRect.reduced (2));
    gainLabel.setBounds (knobArea.getX(), knobRect.getBottom() - 2, knobArea.getWidth(), 16);

    // Small VU meter, limited height
    auto vuArea = bounds.reduced (8, 4);
    int maxVuH = std::min (vuArea.getHeight(), (int) (knobRect.getHeight() * 0.6f));
    int maxVuW = std::min (vuArea.getWidth(), 40);
    vuMeter.setBounds (vuArea.withSizeKeepingCentre (maxVuW, maxVuH));
}

void OutputPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (InstaShadowLookAndFeel::bgMedium.withAlpha (0.5f));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (InstaShadowLookAndFeel::bgLight.withAlpha (0.3f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}
