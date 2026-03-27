#include "LookAndFeel.h"
#include "BinaryData.h"

InstaShadowLookAndFeel::InstaShadowLookAndFeel()
{
    typefaceRegular = juce::Typeface::createSystemTypefaceFor (
        BinaryData::RajdhaniRegular_ttf, BinaryData::RajdhaniRegular_ttfSize);
    typefaceMedium = juce::Typeface::createSystemTypefaceFor (
        BinaryData::RajdhaniMedium_ttf, BinaryData::RajdhaniMedium_ttfSize);
    typefaceBold = juce::Typeface::createSystemTypefaceFor (
        BinaryData::RajdhaniBold_ttf, BinaryData::RajdhaniBold_ttfSize);

    setColour (juce::ResizableWindow::backgroundColourId, bgDark);
    setColour (juce::Label::textColourId, textPrimary);
    setColour (juce::TextButton::buttonColourId, bgMedium);
    setColour (juce::TextButton::textColourOffId, textPrimary);
    setColour (juce::ComboBox::backgroundColourId, bgMedium);
    setColour (juce::ComboBox::textColourId, textPrimary);
    setColour (juce::ComboBox::outlineColourId, bgLight);
    setColour (juce::PopupMenu::backgroundColourId, bgMedium);
    setColour (juce::PopupMenu::textColourId, textPrimary);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, bgLight);

    generateNoiseTexture();
}

juce::Typeface::Ptr InstaShadowLookAndFeel::getTypefaceForFont (const juce::Font& font)
{
    if (font.isBold())
        return typefaceBold;
    return typefaceRegular;
}

juce::Font InstaShadowLookAndFeel::getRegularFont (float height) const
{
    return juce::Font (juce::FontOptions (typefaceRegular).withHeight (height));
}

juce::Font InstaShadowLookAndFeel::getMediumFont (float height) const
{
    return juce::Font (juce::FontOptions (typefaceMedium).withHeight (height));
}

juce::Font InstaShadowLookAndFeel::getBoldFont (float height) const
{
    return juce::Font (juce::FontOptions (typefaceBold).withHeight (height));
}

void InstaShadowLookAndFeel::generateNoiseTexture()
{
    const int texW = 256, texH = 256;
    noiseTexture = juce::Image (juce::Image::ARGB, texW, texH, true);

    juce::Random rng (42);

    for (int y = 0; y < texH; ++y)
    {
        for (int x = 0; x < texW; ++x)
        {
            float noise = rng.nextFloat() * 0.06f;
            bool crossA = ((x + y) % 4 == 0);
            bool crossB = ((x - y + 256) % 4 == 0);
            float pattern = (crossA || crossB) ? 0.03f : 0.0f;
            float alpha = noise + pattern;
            noiseTexture.setPixelAt (x, y, juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, alpha));
        }
    }
}

void InstaShadowLookAndFeel::drawBackgroundTexture (juce::Graphics& g, juce::Rectangle<int> area)
{
    for (int y = area.getY(); y < area.getBottom(); y += noiseTexture.getHeight())
        for (int x = area.getX(); x < area.getRight(); x += noiseTexture.getWidth())
            g.drawImageAt (noiseTexture, x, y);
}

// ============================================================
// Rotary slider (3D metal knob)
// ============================================================

void InstaShadowLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPos, float rotaryStartAngle,
                                               float rotaryEndAngle, juce::Slider& slider)
{
    float knobSize = std::min ((float) width, (float) height);
    float s = knobSize / 60.0f;
    float margin = std::max (4.0f, 6.0f * s);

    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (margin);
    auto radius = std::min (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    auto knobType = slider.getProperties() [knobTypeProperty].toString();
    bool isDark = (knobType == "dark");

    juce::Colour arcColour     = isDark ? juce::Colour (0xff4488ff) : juce::Colour (0xffff8833);
    juce::Colour arcBgColour   = isDark ? juce::Colour (0xff1a2a44) : juce::Colour (0xff2a1a0a);
    juce::Colour bodyTop       = isDark ? juce::Colour (0xff3a3a4a) : juce::Colour (0xff5a4a3a);
    juce::Colour bodyBottom    = isDark ? juce::Colour (0xff1a1a2a) : juce::Colour (0xff2a1a0a);
    juce::Colour rimColour     = isDark ? juce::Colour (0xff555566) : juce::Colour (0xff886644);
    juce::Colour highlightCol  = isDark ? juce::Colour (0x33aabbff) : juce::Colour (0x44ffcc88);
    juce::Colour pointerColour = isDark ? juce::Colour (0xff66aaff) : juce::Colour (0xffffaa44);

    float arcW     = std::max (1.5f, 2.5f * s);
    float glowW1   = std::max (3.0f, 10.0f * s);
    float hotW     = std::max (0.8f, 1.2f * s);
    float ptrW     = std::max (1.2f, 2.0f * s);
    float bodyRadius = radius * 0.72f;

    // 1. Drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (cx - bodyRadius + 1, cy - bodyRadius + 2, bodyRadius * 2, bodyRadius * 2);

    // 2. Outer arc track
    {
        juce::Path arcBg;
        arcBg.addCentredArc (cx, cy, radius - 1, radius - 1, 0.0f,
                              rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (arcBgColour);
        g.strokePath (arcBg, juce::PathStrokeType (arcW, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // 3. Outer arc value with glow
    if (sliderPos > 0.01f)
    {
        juce::Path arcVal;
        arcVal.addCentredArc (cx, cy, radius - 1, radius - 1, 0.0f,
                               rotaryStartAngle, angle, true);

        const int numGlowLayers = 8;
        for (int i = 0; i < numGlowLayers; ++i)
        {
            float t = (float) i / (float) (numGlowLayers - 1);
            float layerWidth = glowW1 * (1.0f - t * 0.7f);
            float layerAlpha = 0.03f + t * t * 0.35f;
            g.setColour (arcColour.withAlpha (layerAlpha));
            g.strokePath (arcVal, juce::PathStrokeType (layerWidth, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
        }

        g.setColour (arcColour);
        g.strokePath (arcVal, juce::PathStrokeType (arcW, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

        g.setColour (arcColour.brighter (0.6f).withAlpha (0.5f));
        g.strokePath (arcVal, juce::PathStrokeType (hotW, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }

    // 4. Knob body
    {
        juce::ColourGradient bodyGrad (bodyTop, cx, cy - bodyRadius * 0.5f,
                                       bodyBottom, cx, cy + bodyRadius, true);
        g.setGradientFill (bodyGrad);
        g.fillEllipse (cx - bodyRadius, cy - bodyRadius, bodyRadius * 2, bodyRadius * 2);
    }

    // 5. Rim
    g.setColour (rimColour.withAlpha (0.6f));
    g.drawEllipse (cx - bodyRadius, cy - bodyRadius, bodyRadius * 2, bodyRadius * 2, std::max (0.8f, 1.2f * s));

    // 6. Inner shadow
    {
        float innerR = bodyRadius * 0.85f;
        juce::ColourGradient innerGrad (juce::Colours::black.withAlpha (0.15f), cx, cy - innerR * 0.3f,
                                         juce::Colours::transparentBlack, cx, cy + innerR, true);
        g.setGradientFill (innerGrad);
        g.fillEllipse (cx - innerR, cy - innerR, innerR * 2, innerR * 2);
    }

    // 7. Top highlight
    {
        float hlRadius = bodyRadius * 0.55f;
        float hlY = cy - bodyRadius * 0.35f;
        juce::ColourGradient hlGrad (highlightCol, cx, hlY - hlRadius * 0.5f,
                                      juce::Colours::transparentBlack, cx, hlY + hlRadius, true);
        g.setGradientFill (hlGrad);
        g.fillEllipse (cx - hlRadius, hlY - hlRadius * 0.6f, hlRadius * 2, hlRadius * 1.2f);
    }

    // 8. Pointer with glow
    {
        float pointerLen = bodyRadius * 0.75f;

        for (int i = 0; i < 4; ++i)
        {
            float t = (float) i / 3.0f;
            float gw = ptrW * (2.0f - t * 1.5f);
            float alpha = 0.02f + t * t * 0.15f;

            juce::Path glowLayer;
            glowLayer.addRoundedRectangle (-gw, -pointerLen, gw * 2, pointerLen * 0.55f, gw * 0.5f);
            glowLayer.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
            g.setColour (pointerColour.withAlpha (alpha));
            g.fillPath (glowLayer);
        }

        {
            juce::Path pointer;
            pointer.addRoundedRectangle (-ptrW * 0.5f, -pointerLen, ptrW, pointerLen * 0.55f, ptrW * 0.5f);
            pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
            g.setColour (pointerColour);
            g.fillPath (pointer);
        }

        {
            juce::Path hotCenter;
            float hw = ptrW * 0.3f;
            hotCenter.addRoundedRectangle (-hw, -pointerLen, hw * 2, pointerLen * 0.5f, hw);
            hotCenter.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));
            g.setColour (pointerColour.brighter (0.7f).withAlpha (0.6f));
            g.fillPath (hotCenter);
        }
    }

    // 9. Center cap
    {
        float capR = bodyRadius * 0.18f;
        juce::ColourGradient capGrad (rimColour.brighter (0.3f), cx, cy - capR,
                                       bodyBottom, cx, cy + capR, false);
        g.setGradientFill (capGrad);
        g.fillEllipse (cx - capR, cy - capR, capR * 2, capR * 2);
    }
}

// ============================================================
// Button style
// ============================================================

void InstaShadowLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                    const juce::Colour& backgroundColour,
                                                    bool shouldDrawButtonAsHighlighted,
                                                    bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);

    auto baseColour = backgroundColour;
    if (shouldDrawButtonAsDown)
        baseColour = baseColour.brighter (0.2f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter (0.1f);

    juce::ColourGradient grad (baseColour.brighter (0.05f), 0, bounds.getY(),
                                baseColour.darker (0.1f), 0, bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (bgLight.withAlpha (shouldDrawButtonAsHighlighted ? 0.8f : 0.5f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

// ============================================================
// Toggle button — glowing switch
// ============================================================

void InstaShadowLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                               bool shouldDrawButtonAsHighlighted,
                                               bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat();
    float h = std::min (bounds.getHeight() * 0.6f, 14.0f);
    float w = h * 1.8f;
    float trackR = h * 0.5f;

    float sx = bounds.getX() + (bounds.getWidth() - w) * 0.5f;
    float sy = bounds.getCentreY() - h * 0.5f;
    auto trackBounds = juce::Rectangle<float> (sx, sy, w, h);

    bool isOn = button.getToggleState();
    auto onColour = accent;
    auto offColour = bgLight;

    float targetPos = isOn ? 1.0f : 0.0f;
    float animPos = (float) button.getProperties().getWithDefault ("animPos", targetPos);
    animPos += (targetPos - animPos) * 0.25f;
    if (std::abs (animPos - targetPos) < 0.01f) animPos = targetPos;
    button.getProperties().set ("animPos", animPos);

    if (std::abs (animPos - targetPos) > 0.005f)
        button.repaint();

    float thumbR = h * 0.4f;
    float thumbX = sx + trackR + animPos * (w - trackR * 2);
    float thumbY = sy + h * 0.5f;
    float glowIntensity = animPos;

    if (glowIntensity > 0.01f)
    {
        for (int i = 0; i < 3; ++i)
        {
            float t = (float) i / 2.0f;
            float expand = (1.0f - t) * 1.5f;
            float alpha = (0.04f + t * t * 0.1f) * glowIntensity;
            g.setColour (onColour.withAlpha (alpha));
            g.fillRoundedRectangle (trackBounds.expanded (expand), trackR + expand);
        }
    }

    {
        juce::Colour offCol = offColour.withAlpha (0.3f);
        juce::Colour onCol  = onColour.withAlpha (0.35f);
        juce::Colour trackCol = offCol.interpolatedWith (onCol, glowIntensity);
        if (shouldDrawButtonAsHighlighted)
            trackCol = trackCol.brighter (0.15f);
        g.setColour (trackCol);
        g.fillRoundedRectangle (trackBounds, trackR);

        g.setColour (offColour.withAlpha (0.4f).interpolatedWith (onColour.withAlpha (0.5f), glowIntensity));
        g.drawRoundedRectangle (trackBounds, trackR, 0.8f);
    }

    if (glowIntensity > 0.01f)
    {
        for (int i = 0; i < 3; ++i)
        {
            float t = (float) i / 2.0f;
            float r = thumbR * (1.5f - t * 0.5f);
            float alpha = (0.05f + t * t * 0.12f) * glowIntensity;
            g.setColour (onColour.withAlpha (alpha));
            g.fillEllipse (thumbX - r, thumbY - r, r * 2, r * 2);
        }
    }

    {
        juce::Colour thumbTopOff  (0xff555566), thumbBotOff  (0xff333344);
        juce::Colour thumbTopOn = onColour.brighter (0.3f), thumbBotOn = onColour.darker (0.2f);
        juce::ColourGradient thumbGrad (
            thumbTopOff.interpolatedWith (thumbTopOn, glowIntensity), thumbX, thumbY - thumbR,
            thumbBotOff.interpolatedWith (thumbBotOn, glowIntensity), thumbX, thumbY + thumbR, false);
        g.setGradientFill (thumbGrad);
        g.fillEllipse (thumbX - thumbR, thumbY - thumbR, thumbR * 2, thumbR * 2);

        g.setColour (juce::Colour (0xff666677).withAlpha (0.5f).interpolatedWith (onColour.withAlpha (0.6f), glowIntensity));
        g.drawEllipse (thumbX - thumbR, thumbY - thumbR, thumbR * 2, thumbR * 2, 0.8f);

        float hlR = thumbR * 0.4f;
        g.setColour (juce::Colours::white.withAlpha (0.1f + 0.15f * glowIntensity));
        g.fillEllipse (thumbX - hlR, thumbY - thumbR * 0.6f - hlR * 0.3f, hlR * 2, hlR * 1.2f);
    }
}

// ============================================================
// ComboBox style
// ============================================================

void InstaShadowLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                           int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                           juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height);

    g.setColour (bgMedium);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (bgLight.withAlpha (0.5f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    // Arrow
    float arrowSize = height * 0.3f;
    float arrowX = (float) width - height * 0.6f;
    float arrowY = ((float) height - arrowSize) * 0.5f;

    juce::Path arrow;
    arrow.addTriangle (arrowX, arrowY,
                       arrowX + arrowSize, arrowY,
                       arrowX + arrowSize * 0.5f, arrowY + arrowSize);
    g.setColour (textSecondary);
    g.fillPath (arrow);
}
