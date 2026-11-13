#include "Meters.h"

namespace
{
    // Needle ballistics from v1.1 (per 30 Hz tick).
    constexpr float grSpring = 0.35f, grDamping = 0.55f;
    constexpr float vuSpring = 0.12f, vuDamping = 0.70f;

    constexpr float restEpsilon   = 0.0005f;  // needle position (0..1 of the sweep)
    constexpr float repaintEpsilon = 0.0015f; // 0.15 degree: about 1/4 pixel at the tip at 2x

    // dB -> degrees from vertical (the mockups' dial).
    constexpr float lawDb[]  = { -20.0f, -10.0f, -7.0f, -5.0f, -3.0f, -1.0f, 0.0f, 2.0f, 3.0f };
    constexpr float lawDeg[] = { -50.0f, -34.0f, -20.0f, -8.0f, 4.0f, 16.0f, 30.0f, 40.0f, 50.0f };
    constexpr int lawSize = 9;

    struct ScaleLabel { const char* text; float x, y; bool red; };
    constexpr ScaleLabel scaleLabels[] = {
        { "20", 24.9f, 68.0f, false }, { "10", 45.2f, 49.8f, false }, { "7", 66.5f, 38.9f, false },
        { "5", 86.4f, 34.0f, false },  { "3", 106.9f, 33.2f, false }, { "1", 127.0f, 36.8f, false },
        { "0", 149.0f, 46.1f, false }, { "+3", 175.1f, 68.0f, true }
    };

    constexpr float pivotX = 100.0f, pivotY = 128.0f;

    juce::Point<float> onDial (float radius, float degrees)
    {
        auto a = juce::degreesToRadians (degrees);
        return { pivotX + radius * std::sin (a), pivotY - radius * std::cos (a) };
    }

    float linearToDb (float level) { return level > 1.0e-6f ? 20.0f * std::log10 (level) : -120.0f; }

    const juce::String& infinityReadout()
    {
        static const juce::String s = minusSign() + juce::String::charToString ((juce::juce_wchar) 0x221e);
        return s;
    }
}

float VuLaw::angleForDb (float db)
{
    if (db <= lawDb[0]) return lawDeg[0];
    for (int i = 1; i < lawSize; ++i)
        if (db <= lawDb[i])
            return juce::jmap (db, lawDb[i - 1], lawDb[i], lawDeg[i - 1], lawDeg[i]);
    return lawDeg[lawSize - 1] + (db - lawDb[lawSize - 1]) * 10.0f;
}

float VuLaw::dbForAngle (float deg)
{
    if (deg <= lawDeg[0]) return lawDb[0];
    for (int i = 1; i < lawSize; ++i)
        if (deg <= lawDeg[i])
            return juce::jmap (deg, lawDeg[i - 1], lawDeg[i], lawDb[i - 1], lawDb[i]);
    return lawDb[lawSize - 1] + (deg - lawDeg[lawSize - 1]) / 10.0f;
}

// ============================================================
// NeedleMeter
// ============================================================

NeedleMeter::NeedleMeter (juce::String caps, juce::String title)
    : stageCaps (std::move (caps)), stageTitle (std::move (title))
{
    setOpaque (true);
    setTooltip ("Click to switch between gain reduction and input level");
    reset();
}

void NeedleMeter::setTheme (ThemeId t)
{
    theme = t;
    invalidateCache();
    repaint();
}

void NeedleMeter::setMode (MeterMode m, const juce::String& inputChannelName)
{
    mode = m;
    inputName = inputChannelName;
    invalidateCache();
    reset();
    repaint();
}

float NeedleMeter::positionForDb (float db) const
{
    return juce::jlimit (0.0f, 1.05f, (VuLaw::angleForDb (db) + 50.0f) / 100.0f);
}

void NeedleMeter::reset()
{
    // Gain reduction rests at 0 dB; input level rests at the left end.
    target = pos = (mode == MeterMode::gainReduction) ? positionForDb (0.0f) : 0.0f;
    vel = 0.0f;
    readout = makeReadout();
}

bool NeedleMeter::update (float valueDb)
{
    target = positionForDb (valueDb);

    bool atRest = std::abs (target - pos) < restEpsilon && std::abs (vel) < restEpsilon;
    if (atRest)
    {
        pos = target;
        vel = 0.0f;
    }
    else
    {
        bool heavy = mode == MeterMode::inputLevel;
        float spring  = heavy ? vuSpring  : grSpring;
        float damping = heavy ? vuDamping : grDamping;
        vel = vel * (1.0f - damping) + spring * (target - pos);
        pos = juce::jlimit (0.0f, 1.05f, pos + vel);
    }

    readout = makeReadout();
    if (std::abs (pos - paintedPos) >= repaintEpsilon || readout != paintedReadout)
        requestRepaint();

    return ! atRest;
}

juce::String NeedleMeter::makeReadout() const
{
    if (mode == MeterMode::inputLevel && pos <= 0.0005f)
        return infinityReadout() + " dB";

    auto db = VuLaw::dbForAngle (pos * 100.0f - 50.0f);
    return formatNumber (db, 1, mode == MeterMode::inputLevel) + " dB";
}

void NeedleMeter::requestRepaint()
{
    paintedPos = pos;
    paintedReadout = readout;
    if (repaintCounter != nullptr)
        ++repaintCounter->count;
    repaint();
}

void NeedleMeter::mouseUp (const juce::MouseEvent& e)
{
    if (onClick && ! e.mouseWasDraggedSinceMouseDown())
        onClick();
}

juce::Rectangle<int> NeedleMeter::faceBoundsFor (ThemeId t, juce::Rectangle<int> c)
{
    if (t == ThemeId::ivory)
        return c.withHeight (juce::roundToInt ((float) c.getWidth() * 151.0f / 196.0f)).reduced (7);
    return c.withHeight (juce::roundToInt ((float) c.getWidth() * 0.75f));
}

juce::Rectangle<float> NeedleMeter::faceRect() const
{
    return faceBoundsFor (theme, getLocalBounds()).toFloat();
}

juce::String NeedleMeter::captionText() const
{
    return mode == MeterMode::gainReduction ? stageCaps + " GR" : inputName.toUpperCase();
}

void NeedleMeter::buildCache (float scale)
{
    auto w = juce::jmax (1, juce::roundToInt ((float) getWidth() * scale));
    auto h = juce::jmax (1, juce::roundToInt ((float) getHeight() * scale));

    under = juce::Image (juce::Image::ARGB, w, h, true);
    {
        juce::Graphics g (under);
        g.addTransform (juce::AffineTransform::scale (scale));
        drawUnder (g);
    }

    over = juce::Image (juce::Image::ARGB, w, h, true);
    {
        juce::Graphics g (over);
        g.addTransform (juce::AffineTransform::scale (scale));
        drawOver (g);
    }

    cacheScale = scale;
}

void NeedleMeter::drawUnder (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto face = faceRect();
    bool ivory = theme == ThemeId::ivory;

    // Background around the face
    g.fillAll (ivory ? Ivory::face : Graphite::meterBox);

    if (ivory)
    {
        auto bezel = face.expanded (7.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillRoundedRectangle (bezel.translated (0.0f, 2.0f), 8.0f);
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.fillRoundedRectangle (bezel.translated (0.0f, -1.0f), 8.0f);
        g.setColour (Ivory::bezel);
        g.fillRoundedRectangle (bezel, 8.0f);

        drawText (g, captionText(), fonts().get (FontId::plexCondMedium, 10.0f, 0.24f), Ivory::ink,
                  { bounds.getX() + 2.0f, bezel.getBottom() + 6.0f, bounds.getWidth(), 13.0f }, juce::Justification::centred);
    }
    else
    {
        drawText (g, captionText(), fonts().get (FontId::interRegular, 10.0f, 0.2f), Graphite::dim,
                  { 0.0f, face.getBottom() + 8.0f, bounds.getWidth(), 13.0f });
    }

    // The face, in the mockup's 200 x 150 coordinates
    auto radius = ivory ? 4.0f : 6.0f;
    juce::Path clip;
    clip.addRoundedRectangle (face, radius);
    g.saveState();
    g.reduceClipRegion (clip);
    g.addTransform (juce::AffineTransform::scale (face.getWidth() / 200.0f).translated (face.getPosition()));

    auto inner = ivory ? juce::Colour (0xfffdf6e3) : juce::Colour (0xfffbeecb);
    auto outer = ivory ? juce::Colour (0xffeadcb9) : juce::Colour (0xffe6d09d);
    auto cy = ivory ? 112.5f : 105.0f;
    g.setGradientFill (juce::ColourGradient (inner, 100.0f, cy, outer, 100.0f + (ivory ? 123.0f : 116.0f), cy, true));
    g.fillRect (0.0f, 0.0f, 200.0f, 150.0f);

    // Inner shadow, heavier at the top
    auto shade = ivory ? juce::Colour (60, 40, 10) : juce::Colour (80, 55, 15);
    g.setGradientFill (juce::ColourGradient (shade.withAlpha (0.35f), 0.0f, 0.0f, shade.withAlpha (0.0f), 0.0f, 22.0f, false));
    g.fillRect (0.0f, 0.0f, 200.0f, 22.0f);
    g.setGradientFill (juce::ColourGradient (shade.withAlpha (0.12f), 0.0f, 0.0f, shade.withAlpha (0.0f), 8.0f, 0.0f, false));
    g.fillRect (0.0f, 0.0f, 8.0f, 150.0f);
    g.setGradientFill (juce::ColourGradient (shade.withAlpha (0.12f), 200.0f, 0.0f, shade.withAlpha (0.0f), 192.0f, 0.0f, false));
    g.fillRect (192.0f, 0.0f, 8.0f, 150.0f);

    auto ink = ivory ? juce::Colour (0xff1f1b16) : juce::Colour (0xff2b2418);
    auto red = ivory ? juce::Colour (0xffa8322a) : juce::Colour (0xffb8442c);
    auto stroke = ivory ? 1.3f : 1.4f;

    juce::Path arc;
    arc.addCentredArc (pivotX, pivotY, 76.0f, 76.0f, 0.0f, juce::degreesToRadians (-50.0f), juce::degreesToRadians (50.0f), true);
    g.setColour (ink);
    g.strokePath (arc, juce::PathStrokeType (stroke));

    juce::Path redArc;
    redArc.addCentredArc (pivotX, pivotY, 80.0f, 80.0f, 0.0f, juce::degreesToRadians (VuLaw::angleForDb (0.0f)),
                          juce::degreesToRadians (51.5f), true);
    g.setColour (red);
    g.strokePath (redArc, juce::PathStrokeType (5.0f));

    g.setColour (ink);
    for (auto db : lawDb)
        g.drawLine ({ onDial (76.0f, VuLaw::angleForDb (db)), onDial (86.0f, VuLaw::angleForDb (db)) }, stroke);

    auto labelFont = ivory ? fonts().get (FontId::plexCondRegular, 9.5f) : fonts().get (FontId::barlowMedium, 10.0f);
    g.setFont (labelFont);
    for (auto& l : scaleLabels)
    {
        g.setColour (l.red ? red : ink);
        g.drawSingleLineText (l.text, juce::roundToInt (l.x), juce::roundToInt (l.y), juce::Justification::horizontallyCentred);
    }

    // Dial legend
    if (ivory)
    {
        g.setFont (fonts().get (FontId::playfairItalic, 12.0f));
        g.setColour (ink);
        g.drawSingleLineText (mode == MeterMode::gainReduction ? stageTitle : inputName, 100, 92,
                              juce::Justification::horizontallyCentred);
    }
    else
    {
        g.setFont (fonts().get (FontId::barlowMedium, 10.0f, 0.2f));
        g.setColour (juce::Colour (0xff5b4d2e));
        g.drawSingleLineText (mode == MeterMode::gainReduction ? "GR dB" : "dBFS", 101, 92,
                              juce::Justification::horizontallyCentred);
    }

    g.restoreState();

    if (! ivory)
    {
        g.setColour (juce::Colour (0xff5b4d2e));
        g.drawRoundedRectangle (face.reduced (0.5f), radius, 1.0f);
    }
}

void NeedleMeter::drawOver (juce::Graphics& g)
{
    auto face = faceRect();
    bool ivory = theme == ThemeId::ivory;

    juce::Path clip;
    clip.addRoundedRectangle (face, ivory ? 4.0f : 6.0f);
    g.reduceClipRegion (clip);
    g.addTransform (juce::AffineTransform::scale (face.getWidth() / 200.0f).translated (face.getPosition()));

    // Cover plate hiding the needle pivot
    juce::Path plate;
    plate.startNewSubPath (0.0f, 150.0f);
    plate.lineTo (0.0f, 116.0f);
    plate.quadraticTo (100.0f, 96.0f, 200.0f, 116.0f);
    plate.lineTo (200.0f, 150.0f);
    plate.closeSubPath();
    g.setColour (ivory ? Ivory::bezel : juce::Colour (0xff2b2418));
    g.fillPath (plate);

    if (! ivory)
    {
        g.setFont (fonts().get (FontId::barlowMedium, 11.0f, 0.27f));
        g.setColour (juce::Colour (0xffe6d09d));
        g.drawSingleLineText (mode == MeterMode::gainReduction ? stageCaps : inputName.toUpperCase(), 101, 135,
                              juce::Justification::horizontallyCentred);
    }
}

void NeedleMeter::paint (juce::Graphics& g)
{
    auto scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (! juce::approximatelyEqual (cacheScale, scale) || under.isNull())
        buildCache (scale);

    auto toLogical = juce::AffineTransform::scale (1.0f / scale);
    g.drawImageTransformed (under, toLogical);

    auto face = faceRect();
    bool ivory = theme == ThemeId::ivory;
    {
        juce::Graphics::ScopedSaveState s (g);
        juce::Path clip;
        clip.addRoundedRectangle (face, ivory ? 4.0f : 6.0f);
        g.reduceClipRegion (clip);
        g.addTransform (juce::AffineTransform::scale (face.getWidth() / 200.0f).translated (face.getPosition()));

        auto angle = pos * 100.0f - 50.0f;
        auto tip = onDial (92.0f, angle);
        g.setColour (ivory ? juce::Colour (0xff14110e) : juce::Colour (0xff1a1510));
        juce::Path needle;
        needle.startNewSubPath (pivotX, pivotY);
        needle.lineTo (tip);
        g.strokePath (needle, juce::PathStrokeType (ivory ? 1.5f : 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.drawImageTransformed (over, toLogical);

    if (ivory)
    {
        juce::Graphics::ScopedSaveState s (g);
        g.addTransform (juce::AffineTransform::scale (face.getWidth() / 200.0f).translated (face.getPosition()));
        g.setFont (fonts().get (FontId::plexMonoMedium, 12.0f));
        g.setColour (Ivory::displayText);
        g.drawSingleLineText (readout, 100, 136, juce::Justification::horizontallyCentred);
    }
    else
    {
        drawText (g, readout, fonts().get (FontId::plexMonoMedium, 10.0f), Graphite::goldLight,
                  { 0.0f, face.getBottom() + 8.0f, (float) getWidth(), 13.0f }, juce::Justification::centredRight);
    }
}

// ============================================================
// SegmentMeter
// ============================================================

SegmentMeter::SegmentMeter (juce::Colour a) : accent (a)
{
    setOpaque (true);
    setTooltip ("Click to switch between gain reduction and input level");
    reset();
}

void SegmentMeter::setMode (MeterMode m, const juce::String& inputChannelName)
{
    mode = m;
    inputName = inputChannelName;
    reset();
    repaint();
}

void SegmentMeter::reset()
{
    amount = 0.0f;
    litSegments = computeLit();
    readout = makeReadout();
}

int SegmentMeter::computeLit() const
{
    if (mode == MeterMode::gainReduction)
        return juce::jlimit (0, 20, juce::roundToInt (amount));
    return juce::jlimit (0, 20, juce::roundToInt (linearToDb (amount) + 20.0f));
}

juce::String SegmentMeter::makeReadout() const
{
    if (mode == MeterMode::gainReduction)
        return formatNumber (-amount, 1, false);
    auto db = linearToDb (amount);
    return db < -60.0f ? infinityReadout() : formatNumber (db, 1, false);
}

bool SegmentMeter::update (float valueDb)
{
    bool moving;
    if (mode == MeterMode::gainReduction)
    {
        // Instant attack, exponential fall (as the v1.1 bar meter).
        auto gr = juce::jlimit (0.0f, 30.0f, -valueDb);
        auto previous = amount;
        amount = std::max (gr, amount * 0.92f);
        if (amount < 0.01f) amount = 0.0f;
        moving = std::abs (amount - previous) > 0.005f;   // still rising or falling
    }
    else
    {
        auto level = valueDb > -120.0f ? juce::Decibels::decibelsToGain (valueDb) : 0.0f;
        auto previous = amount;
        amount = std::max (level, amount * 0.85f);
        if (amount < 1.0e-4f) amount = 0.0f;
        moving = std::abs (amount - previous) > 1.0e-4f;
    }

    auto lit = computeLit();
    auto text = makeReadout();
    if (lit != litSegments || text != readout)
    {
        litSegments = lit;
        readout = text;
        if (repaintCounter != nullptr)
            ++repaintCounter->count;
        repaint();
    }
    return moving;
}

void SegmentMeter::mouseUp (const juce::MouseEvent& e)
{
    if (onClick && ! e.mouseWasDraggedSinceMouseDown())
        onClick();
}

void SegmentMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.fillAll (Console::panel);

    auto title = mode == MeterMode::gainReduction ? juce::String ("Gain reduction") : "Input level " + inputName.getLastCharacters (1);
    drawText (g, title, fonts().get (FontId::interRegular, 10.5f), Console::dim, { 0.0f, 9.0f, 150.0f, 14.0f });
    drawText (g, readout, fonts().get (FontId::jetMonoMedium, 20.0f), accent,
              { b.getRight() - 110.0f, 0.0f, 110.0f, 26.0f }, juce::Justification::centredRight);

    const float gap = 2.0f, top = 32.0f, h = 10.0f;
    auto w = (b.getWidth() - gap * 19.0f) / 20.0f;
    for (int i = 0; i < 20; ++i)
    {
        // GR fills from the right (0 dB) to the left; input level from the left.
        bool lit = mode == MeterMode::gainReduction ? (19 - i) < litSegments : i < litSegments;
        g.setColour (lit ? accent : Console::segOff);
        g.fillRect (b.getX() + (float) i * (w + gap), top, w, h);
    }

    auto font = fonts().get (FontId::jetMonoRegular, 9.0f);
    auto ly = top + h + 6.0f;
    auto xAt = [&] (int seg) { return b.getX() + (float) seg * (w + gap); };
    drawText (g, minusSign() + "20", font, Console::dimmer, { b.getX(), ly, 30.0f, 12.0f });
    drawText (g, minusSign() + "10", font, Console::dimmer, { xAt (10) - 15.0f, ly, 30.0f, 12.0f }, juce::Justification::centred);
    drawText (g, minusSign() + "5", font, Console::dimmer, { xAt (15) - 15.0f, ly, 30.0f, 12.0f }, juce::Justification::centred);
    drawText (g, "0", font, Console::dimmer, { b.getRight() - 30.0f, ly, 30.0f, 12.0f }, juce::Justification::centredRight);
}

// ============================================================
// OutputMeter
// ============================================================

OutputMeter::OutputMeter()
{
    setOpaque (true);
    setInterceptsMouseClicks (false, false);
}

void OutputMeter::reset()
{
    level[0] = level[1] = 0.0f;
    shown[0] = shown[1] = 0;
    repaint();
}

int OutputMeter::quantise (float l) const
{
    if (theme == ThemeId::ivory)
    {
        static constexpr float thresholds[] = { -40.0f, -30.0f, -24.0f, -18.0f, -12.0f, -8.0f, -5.0f, -2.0f, 0.0f };
        auto db = linearToDb (l);
        int lit = 0;
        for (auto t : thresholds)
            if (db >= t - 0.05f) ++lit;
        return lit;
    }

    // Bar height in quarter pixels of the design size.
    auto display = std::sqrt (juce::jlimit (0.0f, 1.0f, l));
    return juce::roundToInt (display * (float) getHeight() * 4.0f);
}

bool OutputMeter::update (float left, float right)
{
    const float in[2] = { left, right };
    bool moving = false;
    bool changed = false;

    for (int ch = 0; ch < 2; ++ch)
    {
        auto previous = level[ch];
        level[ch] = std::max (in[ch], level[ch] * 0.85f);
        if (level[ch] < 1.0e-4f) level[ch] = 0.0f;
        moving = moving || std::abs (level[ch] - previous) > 1.0e-4f;

        auto q = quantise (level[ch]);
        if (q != shown[ch]) { shown[ch] = q; changed = true; }
    }

    if (changed)
    {
        if (repaintCounter != nullptr)
            ++repaintCounter->count;
        repaint();
    }
    return moving;
}

void OutputMeter::paintBar (juce::Graphics& g, juce::Rectangle<float> bar, float heightPx)
{
    bool console = theme == ThemeId::console;
    g.setColour (console ? Console::inner : Graphite::inset);
    g.fillRoundedRectangle (bar, 3.0f);

    if (heightPx <= 0.0f)
        return;

    auto fill = bar.withTop (bar.getBottom() - heightPx);
    juce::ColourGradient grad (console ? Console::cyanDark : juce::Colour (0xff8c7440), 0.0f, fill.getBottom(),
                               console ? Console::orange : Graphite::lamp, 0.0f, fill.getY(), false);
    grad.addColour (console ? 0.85 : 0.80, console ? Console::cyan : Graphite::gold);

    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle (bar, 3.0f);
    g.reduceClipRegion (clip);
    g.setGradientFill (grad);
    g.fillRect (fill);
    g.restoreState();
}

void OutputMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Opaque: paint the panel behind, so a level change repaints only this meter.
    g.fillAll (theme == ThemeId::ivory ? Ivory::face : theme == ThemeId::console ? Console::panel : Graphite::panel);

    if (theme == ThemeId::ivory)
    {
        const float w = 10.0f, h = 7.0f, gap = 3.0f, rowGap = 5.0f;
        for (int row = 0; row < 2; ++row)
            for (int i = 0; i < 9; ++i)
            {
                bool lit = i < shown[row];
                auto colour = ! lit ? Ivory::ledUnlit : i < 6 ? Ivory::ledGreen : i == 6 ? Ivory::ledAmber : Ivory::ledRed;
                g.setColour (colour);
                g.fillRoundedRectangle (b.getX() + (float) i * (w + gap), b.getY() + (float) row * (h + rowGap), w, h, 1.0f);
            }
        return;
    }

    auto barW = theme == ThemeId::console ? 9.0f : 7.0f;
    auto gap = 4.0f;
    for (int ch = 0; ch < 2; ++ch)
        paintBar (g, { b.getX() + (float) ch * (barW + gap), b.getY(), barW, b.getHeight() }, (float) shown[ch] / 4.0f);
}
