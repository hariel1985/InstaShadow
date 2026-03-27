#include "TransformerPanel.h"

TransformerPanel::TransformerPanel()
{
    titleLabel.setText ("TRANSFORMER", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    titleLabel.setColour (juce::Label::textColourId, InstaShadowLookAndFeel::textPrimary);
    addAndMakeVisible (titleLabel);

    auto setupButton = [this] (juce::TextButton& btn, int typeId)
    {
        btn.setClickingTogglesState (false);
        btn.onClick = [this, typeId, &btn]
        {
            if (currentType == typeId)
                currentType = 0;  // deselect → Off
            else
                currentType = typeId;
            updateButtonStates();
        };
        addAndMakeVisible (btn);
    };

    setupButton (nickelButton, 1);
    setupButton (ironButton, 2);
    setupButton (steelButton, 3);

    updateButtonStates();
}

void TransformerPanel::setSelectedType (int type)
{
    currentType = std::clamp (type, 0, 3);
    updateButtonStates();
}

void TransformerPanel::updateButtonStates()
{
    auto setColour = [this] (juce::TextButton& btn, bool active)
    {
        if (active)
        {
            btn.setColour (juce::TextButton::buttonColourId, InstaShadowLookAndFeel::accent.withAlpha (0.4f));
            btn.setColour (juce::TextButton::textColourOffId, InstaShadowLookAndFeel::accent);
        }
        else
        {
            btn.setColour (juce::TextButton::buttonColourId, InstaShadowLookAndFeel::bgMedium);
            btn.setColour (juce::TextButton::textColourOffId, InstaShadowLookAndFeel::textSecondary);
        }
    };

    setColour (nickelButton, currentType == 1);
    setColour (ironButton, currentType == 2);
    setColour (steelButton, currentType == 3);

    repaint();
}

void TransformerPanel::resized()
{
    auto bounds = getLocalBounds().reduced (4);
    titleLabel.setBounds (bounds.removeFromTop (20));

    auto btnArea = bounds.reduced (8, 4);
    int btnH = std::min ((btnArea.getHeight() - 8) / 3, 28);
    int totalH = btnH * 3 + 8;
    auto centred = btnArea.withSizeKeepingCentre (std::min (btnArea.getWidth(), 120), totalH);

    nickelButton.setBounds (centred.removeFromTop (btnH).reduced (0, 1));
    centred.removeFromTop (4);
    ironButton.setBounds (centred.removeFromTop (btnH).reduced (0, 1));
    centred.removeFromTop (4);
    steelButton.setBounds (centred.removeFromTop (btnH).reduced (0, 1));
}

void TransformerPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (InstaShadowLookAndFeel::bgMedium.withAlpha (0.5f));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (InstaShadowLookAndFeel::bgLight.withAlpha (0.3f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}
