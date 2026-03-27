#include "OpticalPanel.h"

OpticalPanel::OpticalPanel()
{
    titleLabel.setText ("OPTICAL", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textPrimary);
    addAndMakeVisible (titleLabel);

    auto setupKnob = [this] (juce::Slider& knob, juce::Label& label, const juce::String& name,
                              double min, double max, double def, const juce::String& type,
                              const juce::String& suffix = "")
    {
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
        knob.setRange (min, max);
        knob.setValue (def);
        knob.setTextValueSuffix (suffix);
        knob.getProperties().set (InstaShadowLookAndFeel::knobTypeProperty, type);
        addAndMakeVisible (knob);

        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textSecondary);
        addAndMakeVisible (label);
    };

    setupKnob (thresholdKnob, threshLabel, "Threshold", -40.0, 0.0, -20.0, "orange", " dB");
    setupKnob (gainKnob, gainLabel, "Gain", 0.0, 20.0, 0.0, "orange", " dB");
    setupKnob (scHpfKnob, hpfLabel, "SC HPF", 20.0, 500.0, 90.0, "dark", " Hz");
    scHpfKnob.setSkewFactorFromMidPoint (100.0);

    bypassToggle.setButtonText ("");
    addAndMakeVisible (bypassToggle);
    bypassLabel.setText ("Bypass", juce::dontSendNotification);
    bypassLabel.setJustificationType (juce::Justification::centred);
    bypassLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textSecondary);
    addAndMakeVisible (bypassLabel);
}

void OpticalPanel::resized()
{
    auto bounds = getLocalBounds().reduced (4);
    titleLabel.setBounds (bounds.removeFromTop (18));

    int availH = bounds.getHeight() - 22;
    int knobH = availH / 3;

    auto k1 = bounds.removeFromTop (knobH);
    thresholdKnob.setBounds (k1.withTrimmedBottom (14).reduced (4, 0));
    threshLabel.setBounds (k1.getX(), k1.getBottom() - 14, k1.getWidth(), 14);

    auto k2 = bounds.removeFromTop (knobH);
    gainKnob.setBounds (k2.withTrimmedBottom (14).reduced (4, 0));
    gainLabel.setBounds (k2.getX(), k2.getBottom() - 14, k2.getWidth(), 14);

    auto k3 = bounds.removeFromTop (knobH);
    scHpfKnob.setBounds (k3.withTrimmedBottom (14).reduced (4, 0));
    hpfLabel.setBounds (k3.getX(), k3.getBottom() - 14, k3.getWidth(), 14);

    auto bpRow = bounds;
    bypassLabel.setBounds (bpRow.removeFromLeft (bpRow.getWidth() / 2));
    bypassToggle.setBounds (bpRow.reduced (4, 2));
}

void OpticalPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (InstaShadowLookAndFeel::bgMedium.withAlpha (0.5f));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (InstaShadowLookAndFeel::bgLight.withAlpha (0.3f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}
