#pragma once
#include <JuceHeader.h>

// ============================================================
// The three editor themes. Each has its own colours, fonts and
// layout (see PluginEditor.cpp); the widgets paint themselves
// according to the theme they are given.
// ============================================================
enum class ThemeId { graphite = 0, console = 1, ivory = 2 };

inline constexpr int numThemes = 3;

// Design size of the editor; the content is laid out at this size
// and scaled to the window.
inline constexpr int designWidth  = 1000;
inline constexpr int designHeight = 540;

// ----- A: Graphite & Brass -----
namespace Graphite
{
    inline const juce::Colour bg          { 0xff17181b };
    inline const juce::Colour header      { 0xff1d1e22 };
    inline const juce::Colour headerLine  { 0xff2c2e33 };
    inline const juce::Colour panel       { 0xff202125 };
    inline const juce::Colour panelBorder { 0xff2e3035 };
    inline const juce::Colour text        { 0xffece6d6 };
    inline const juce::Colour dim         { 0xff9a958a };
    inline const juce::Colour gold        { 0xffc9a45c };
    inline const juce::Colour goldLight   { 0xffe0bf7a };
    inline const juce::Colour lamp        { 0xffe2b155 };
    inline const juce::Colour lampOff     { 0xff3a3b40 };
    inline const juce::Colour btnBg       { 0xff222327 };
    inline const juce::Colour btnBorder   { 0xff34363c };
    inline const juce::Colour inset       { 0xff121315 };
    inline const juce::Colour chevron     { 0xffc8c2b4 };
    inline const juce::Colour cream       { 0xfff1e3c2 };
    inline const juce::Colour track       { 0xff33353b };
    inline const juce::Colour greyArc     { 0xff8a8578 };
    inline const juce::Colour meterBox    { 0xff101113 };
    inline const juce::Colour meterBorder { 0xff2a2c30 };
    inline const juce::Colour selBg       { 0xff2a2418 };
    inline const juce::Colour darkBtn     { 0xff17181b };
    inline const juce::Colour pillBorder  { 0xff3a3b40 };
    inline const juce::Colour saveBorder  { 0xff5a4a28 };
    inline const juce::Colour bypassLamp  { 0xffd2603f };
}

// ----- B: Mastering Console -----
namespace Console
{
    inline const juce::Colour bg          { 0xff0d0e11 };
    inline const juce::Colour line        { 0xff1f2228 };
    inline const juce::Colour panel       { 0xff14161a };
    inline const juce::Colour text        { 0xffe8ebef };
    inline const juce::Colour dim         { 0xff8b93a1 };
    inline const juce::Colour dimmer      { 0xff6b7280 };
    inline const juce::Colour soft        { 0xffb9c0cb };
    inline const juce::Colour cyan        { 0xff6fd3ec };
    inline const juce::Colour cyanSoft    { 0xff9fe3f3 };
    inline const juce::Colour cyanBg      { 0xff102027 };
    inline const juce::Colour cyanBorder  { 0xff2c5560 };
    inline const juce::Colour cyanDark    { 0xff2f8aa0 };
    inline const juce::Colour orange      { 0xfff0a35e };
    inline const juce::Colour orangeBg    { 0xff2a1c10 };
    inline const juce::Colour orangeBorder{ 0xff6a4524 };
    inline const juce::Colour btnBg       { 0xff15171c };
    inline const juce::Colour btnBorder   { 0xff2a2e36 };
    inline const juce::Colour inner       { 0xff1b1e24 };
    inline const juce::Colour innerBorder { 0xff262a32 };
    inline const juce::Colour track       { 0xff262a32 };
    inline const juce::Colour segOff      { 0xff23272f };
    inline const juce::Colour chevron     { 0xff3a3f49 };
    inline const juce::Colour abActive    { 0xff232731 };
    inline const juce::Colour saveText    { 0xff071216 };
}

// ----- C: Ivory Rack -----
namespace Ivory
{
    inline const juce::Colour outer       { 0xff26231f };
    inline const juce::Colour ear         { 0xffd9d1c1 };
    inline const juce::Colour earBorder   { 0xffbdb3a0 };
    inline const juce::Colour slot        { 0xff2e2a25 };
    inline const juce::Colour face        { 0xffece6da };
    inline const juce::Colour line        { 0xffcfc6b4 };
    inline const juce::Colour ink         { 0xff2a2622 };
    inline const juce::Colour inkDark     { 0xff1f1c19 };
    inline const juce::Colour dim         { 0xff6b6358 };
    inline const juce::Colour label       { 0xff4a443c };
    inline const juce::Colour btnBorder   { 0xffb9ae99 };
    inline const juce::Colour btnBg       { 0xffe2dacb };
    inline const juce::Colour display     { 0xff171513 };
    inline const juce::Colour displayEdge { 0xff1d1a17 };
    inline const juce::Colour displayText { 0xfff2d9a6 };
    inline const juce::Colour displayDim  { 0xff8d7a58 };
    inline const juce::Colour red         { 0xff8e2b23 };
    inline const juce::Colour ledOn       { 0xffd23b2c };
    inline const juce::Colour ledGlow     { 0xffe2523f };
    inline const juce::Colour ledOff      { 0xff6f2a24 };
    inline const juce::Colour segBg       { 0xffe4dccd };
    inline const juce::Colour segDivider  { 0xffc3b9a6 };
    inline const juce::Colour segSelText  { 0xfff4efe4 };
    inline const juce::Colour swatchEdge  { 0xffc8beab };
    inline const juce::Colour bezel       { 0xff161412 };
    inline const juce::Colour ledGreen    { 0xff2f7d4f };
    inline const juce::Colour ledAmber    { 0xffc99a2e };
    inline const juce::Colour ledRed      { 0xffa8322a };
    inline const juce::Colour ledUnlit    { 0xffc8beab };
}

// ============================================================
// Embedded fonts (all SIL Open Font License, see Resources/Fonts/licenses)
// ============================================================
enum class FontId
{
    barlowMedium, barlowSemiBold,
    interRegular, interMedium, interSemiBold,
    plexMonoMedium,
    jetMonoRegular, jetMonoMedium,
    plexCondRegular, plexCondMedium, plexCondSemiBold,
    playfairItalic,
    count
};

class Fonts
{
public:
    Fonts();

    // size = CSS pixel size (em), tracking = CSS letter-spacing in em.
    juce::Font get (FontId, float size, float tracking = 0.0f) const;

private:
    std::array<juce::Typeface::Ptr, (size_t) FontId::count> typefaces;
};

// Shared instance (loaded once per process).
const Fonts& fonts();

// Draws a single line of text in a box.
void drawText (juce::Graphics&, const juce::String& text, const juce::Font& font, juce::Colour colour,
               juce::Rectangle<float> box, juce::Justification just = juce::Justification::centredLeft);

// Formats a dB value with a typographic minus sign.
juce::String formatNumber (float value, int decimals, bool showPlus);

// U+2212 MINUS SIGN as a juce::String.
const juce::String& minusSign();

// A small drop shadow under a shape (cheap approximation of a CSS box-shadow).
void drawSoftShadow (juce::Graphics&, const juce::Path& shape, juce::Colour colour, float radius, juce::Point<int> offset);
