#include "DiscretePanel.h"

DiscretePanel::DiscretePanel()
{
    titleLabel.setText ("DISCRETE", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textPrimary);
    addAndMakeVisible (titleLabel);

    auto setupKnob = [this] (juce::Slider& knob, juce::Label& label, const juce::String& name,
                              double min, double max, double def, const juce::String& suffix)
    {
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
        knob.setRange (min, max);
        knob.setValue (def);
        knob.setTextValueSuffix (suffix);
        knob.getProperties().set (InstaShadowLookAndFeel::knobTypeProperty, "orange");
        addAndMakeVisible (knob);

        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textSecondary);
        addAndMakeVisible (label);
    };

    setupKnob (thresholdKnob, threshLabel, "Threshold", -40.0, 0.0, -20.0, " dB");
    setupKnob (gainKnob, gainLabel, "Gain", 0.0, 20.0, 0.0, " dB");

    ratioBox.addItem ("1.2:1", 1);
    ratioBox.addItem ("2:1", 2);
    ratioBox.addItem ("3:1", 3);
    ratioBox.addItem ("4:1", 4);
    ratioBox.addItem ("6:1", 5);
    ratioBox.addItem ("10:1", 6);
    ratioBox.addItem ("Flood", 7);
    ratioBox.setSelectedId (2);
    addAndMakeVisible (ratioBox);
    ratioLabel.setText ("Ratio", juce::dontSendNotification);
    ratioLabel.setJustificationType (juce::Justification::centred);
    ratioLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textSecondary);
    addAndMakeVisible (ratioLabel);

    attackBox.addItem ("0.1 ms", 1);
    attackBox.addItem ("0.5 ms", 2);
    attackBox.addItem ("1 ms", 3);
    attackBox.addItem ("5 ms", 4);
    attackBox.addItem ("10 ms", 5);
    attackBox.addItem ("30 ms", 6);
    attackBox.setSelectedId (3);
    addAndMakeVisible (attackBox);
    attackLabel.setText ("Attack", juce::dontSendNotification);
    attackLabel.setJustificationType (juce::Justification::centred);
    attackLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textSecondary);
    addAndMakeVisible (attackLabel);

    releaseBox.addItem ("100 ms", 1);
    releaseBox.addItem ("250 ms", 2);
    releaseBox.addItem ("500 ms", 3);
    releaseBox.addItem ("800 ms", 4);
    releaseBox.addItem ("1.2 s", 5);
    releaseBox.addItem ("Dual", 6);
    releaseBox.setSelectedId (3);
    addAndMakeVisible (releaseBox);
    releaseLabel.setText ("Release", juce::dontSendNotification);
    releaseLabel.setJustificationType (juce::Justification::centred);
    releaseLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textSecondary);
    addAndMakeVisible (releaseLabel);

    bypassToggle.setButtonText ("");
    addAndMakeVisible (bypassToggle);
    bypassLabel.setText ("Bypass", juce::dontSendNotification);
    bypassLabel.setJustificationType (juce::Justification::centred);
    bypassLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textSecondary);
    addAndMakeVisible (bypassLabel);
}

void DiscretePanel::resized()
{
    auto bounds = getLocalBounds().reduced (4);
    titleLabel.setBounds (bounds.removeFromTop (18));

    int availH = bounds.getHeight() - 22;  // reserve for bypass

    // Top: 2 knobs stacked vertically (~55%)
    int knobSectionH = (int) (availH * 0.55f);
    int singleKnobH = knobSectionH / 2;

    auto k1 = bounds.removeFromTop (singleKnobH);
    thresholdKnob.setBounds (k1.withTrimmedBottom (14).reduced (4, 0));
    threshLabel.setBounds (k1.getX(), k1.getBottom() - 14, k1.getWidth(), 14);

    auto k2 = bounds.removeFromTop (singleKnobH);
    gainKnob.setBounds (k2.withTrimmedBottom (14).reduced (4, 0));
    gainLabel.setBounds (k2.getX(), k2.getBottom() - 14, k2.getWidth(), 14);

    // Gap between knobs and combos
    bounds.removeFromTop (6);

    // Middle: 3 combos stacked (~30%)
    int comboSectionH = (int) (availH * 0.30f);
    int comboH = comboSectionH / 3;

    auto r1 = bounds.removeFromTop (comboH);
    ratioLabel.setBounds (r1.removeFromLeft (r1.getWidth() / 3));
    ratioBox.setBounds (r1.reduced (2, 1));

    auto r2 = bounds.removeFromTop (comboH);
    attackLabel.setBounds (r2.removeFromLeft (r2.getWidth() / 3));
    attackBox.setBounds (r2.reduced (2, 1));

    auto r3 = bounds.removeFromTop (comboH);
    releaseLabel.setBounds (r3.removeFromLeft (r3.getWidth() / 3));
    releaseBox.setBounds (r3.reduced (2, 1));

    // Bottom: bypass
    auto bpRow = bounds;
    bypassLabel.setBounds (bpRow.removeFromLeft (bpRow.getWidth() / 2));
    bypassToggle.setBounds (bpRow.reduced (4, 2));
}

void DiscretePanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (InstaShadowLookAndFeel::bgMedium.withAlpha (0.5f));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (InstaShadowLookAndFeel::bgLight.withAlpha (0.3f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}
