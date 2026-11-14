#include "Theme.h"
#include "BinaryData.h"

Fonts::Fonts()
{
    auto load = [this] (FontId id, const void* data, int size)
    {
        typefaces[(size_t) id] = juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    };

    load (FontId::barlowMedium,     BinaryData::BarlowCondensedMedium_ttf,       BinaryData::BarlowCondensedMedium_ttfSize);
    load (FontId::barlowSemiBold,   BinaryData::BarlowCondensedSemiBold_ttf,     BinaryData::BarlowCondensedSemiBold_ttfSize);
    load (FontId::interRegular,     BinaryData::InterRegular_ttf,                BinaryData::InterRegular_ttfSize);
    load (FontId::interMedium,      BinaryData::InterMedium_ttf,                 BinaryData::InterMedium_ttfSize);
    load (FontId::interSemiBold,    BinaryData::InterSemiBold_ttf,               BinaryData::InterSemiBold_ttfSize);
    load (FontId::plexMonoMedium,   BinaryData::IBMPlexMonoMedium_ttf,           BinaryData::IBMPlexMonoMedium_ttfSize);
    load (FontId::jetMonoRegular,   BinaryData::JetBrainsMonoRegular_ttf,        BinaryData::JetBrainsMonoRegular_ttfSize);
    load (FontId::jetMonoMedium,    BinaryData::JetBrainsMonoMedium_ttf,         BinaryData::JetBrainsMonoMedium_ttfSize);
    load (FontId::plexCondRegular,  BinaryData::IBMPlexSansCondensedRegular_ttf, BinaryData::IBMPlexSansCondensedRegular_ttfSize);
    load (FontId::plexCondMedium,   BinaryData::IBMPlexSansCondensedMedium_ttf,  BinaryData::IBMPlexSansCondensedMedium_ttfSize);
    load (FontId::plexCondSemiBold, BinaryData::IBMPlexSansCondensedSemiBold_ttf,BinaryData::IBMPlexSansCondensedSemiBold_ttfSize);
    load (FontId::playfairItalic,   BinaryData::PlayfairDisplaySemiBoldItalic_ttf, BinaryData::PlayfairDisplaySemiBoldItalic_ttfSize);
}

juce::Font Fonts::get (FontId id, float size, float tracking) const
{
    return juce::Font (juce::FontOptions (typefaces[(size_t) id])
                           .withPointHeight (size)
                           .withKerningFactor (tracking));
}

const Fonts& fonts()
{
    static juce::SharedResourcePointer<Fonts> shared;
    return *shared;
}

void drawText (juce::Graphics& g, const juce::String& text, const juce::Font& font, juce::Colour colour,
               juce::Rectangle<float> box, juce::Justification just)
{
    g.setFont (font);
    g.setColour (colour);
    g.drawText (text, box, just, false);
}

const juce::String& minusSign()
{
    static const juce::String m = juce::String::charToString ((juce::juce_wchar) 0x2212);
    return m;
}

juce::String formatNumber (float value, int decimals, bool showPlus)
{
    auto rounded = std::round (value * std::pow (10.0f, (float) decimals)) / std::pow (10.0f, (float) decimals);
    auto s = juce::String (std::abs (rounded), decimals);
    if (rounded < 0.0f)  return minusSign() + s;
    if (rounded > 0.0f && showPlus) return "+" + s;
    return s;
}

void drawSoftShadow (juce::Graphics& g, const juce::Path& shape, juce::Colour colour, float radius, juce::Point<int> offset)
{
    juce::DropShadow (colour, juce::roundToInt (radius), offset).drawForPath (g, shape);
}
