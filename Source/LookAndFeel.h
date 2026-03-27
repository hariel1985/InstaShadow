#pragma once
#include <JuceHeader.h>

class InstaShadowLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Colour palette
    static inline const juce::Colour bgDark      { 0xff1a1a2e };
    static inline const juce::Colour bgMedium    { 0xff16213e };
    static inline const juce::Colour bgLight     { 0xff0f3460 };
    static inline const juce::Colour textPrimary  { 0xffe0e0e0 };
    static inline const juce::Colour textSecondary { 0xff888899 };
    static inline const juce::Colour accent       { 0xff00ff88 };

    // Knob type property key
    static constexpr const char* knobTypeProperty = "knobType";

    InstaShadowLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    // Custom fonts
    juce::Font getRegularFont (float height) const;
    juce::Font getMediumFont (float height) const;
    juce::Font getBoldFont (float height) const;

    // Background texture
    void drawBackgroundTexture (juce::Graphics& g, juce::Rectangle<int> area);

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override;

private:
    juce::Typeface::Ptr typefaceRegular;
    juce::Typeface::Ptr typefaceMedium;
    juce::Typeface::Ptr typefaceBold;

    juce::Image noiseTexture;
    void generateNoiseTexture();
};
