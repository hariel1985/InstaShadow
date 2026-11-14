#include "Widgets.h"

namespace
{
    constexpr float pi = juce::MathConstants<float>::pi;
    constexpr float startAngle = -0.75f * pi;   // -135 degrees from 12 o'clock
    constexpr float endAngle   =  0.75f * pi;

    juce::Point<float> polar (juce::Point<float> c, float radius, float angle)
    {
        return { c.x + radius * std::sin (angle), c.y - radius * std::cos (angle) };
    }

    void fillRadial (juce::Graphics& g, juce::Rectangle<float> box, float fx, float fy, float reach,
                     juce::Colour inner, juce::Colour outer)
    {
        juce::Point<float> c { box.getX() + box.getWidth() * fx, box.getY() + box.getHeight() * fy };
        juce::ColourGradient grad (inner, c, outer, c.translated (box.getWidth() * reach, 0.0f), true);
        g.setGradientFill (grad);
        g.fillEllipse (box);
    }

    void fillRing (juce::Graphics& g, juce::Rectangle<float> box, float thickness, float from, float to, juce::Colour c)
    {
        if (std::abs (to - from) < 1.0e-4f) return;
        juce::Path p;
        auto inner = (box.getWidth() * 0.5f - thickness) / (box.getWidth() * 0.5f);
        p.addPieSegment (box, std::min (from, to), std::max (from, to), inner);
        g.setColour (c);
        g.fillPath (p);
    }

    juce::Path roundedBox (juce::Rectangle<float> r, float radius, bool left = true, bool right = true)
    {
        juce::Path p;
        p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radius, radius,
                               left, right, left, right);
        return p;
    }

    void fillAndStroke (juce::Graphics& g, const juce::Path& p, juce::Colour fill, juce::Colour stroke, float width = 1.0f)
    {
        g.setColour (fill);
        g.fillPath (p);
        g.setColour (stroke);
        g.strokePath (p, juce::PathStrokeType (width));
    }

    void drawLamp (juce::Graphics& g, juce::Point<float> c, float diameter, juce::Colour colour, bool glow)
    {
        if (glow)
        {
            for (int i = 3; i >= 1; --i)
            {
                auto r = diameter * 0.5f + (float) i * diameter * 0.25f;
                g.setColour (colour.withAlpha (0.10f));
                g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
            }
        }
        g.setColour (colour);
        g.fillEllipse (c.x - diameter * 0.5f, c.y - diameter * 0.5f, diameter, diameter);
    }

    juce::String spacedRatio (const juce::String& s) { return s.replace (":", " : "); }
}

juce::Path chevronPath (juce::Rectangle<float> box, int direction)
{
    juce::Path p;
    auto c = box.getCentre();
    auto s = box.getWidth() / 12.0f;
    if (direction < 0)
    {
        p.startNewSubPath (c.x + 1.5f * s, c.y - 3.5f * s);
        p.lineTo (c.x - 2.0f * s, c.y);
        p.lineTo (c.x + 1.5f * s, c.y + 3.5f * s);
    }
    else if (direction > 0)
    {
        p.startNewSubPath (c.x - 1.5f * s, c.y - 3.5f * s);
        p.lineTo (c.x + 2.0f * s, c.y);
        p.lineTo (c.x - 1.5f * s, c.y + 3.5f * s);
    }
    else
    {
        p.startNewSubPath (c.x - 3.6f * s, c.y - 1.8f * s);
        p.lineTo (c.x, c.y + 1.8f * s);
        p.lineTo (c.x + 3.6f * s, c.y - 1.8f * s);
    }
    return p;
}

// ============================================================
// Knob
// ============================================================

Knob::Knob()
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setRotaryParameters (1.25f * pi, 2.75f * pi, true);
    setMouseDragSensitivity (220);
    setVelocityBasedMode (false);
}

float Knob::angleForProportion (float p) const
{
    return startAngle + juce::jlimit (0.0f, 1.0f, p) * (endAngle - startAngle);
}

juce::Colour Knob::accentColour() const
{
    switch (theme)
    {
        case ThemeId::graphite:
            return accent == Accent::grey ? Graphite::greyArc : Graphite::gold;
        case ThemeId::console:
            switch (accent)
            {
                case Accent::secondary: return Console::orange;
                case Accent::grey:      return Console::dim;
                case Accent::white:     return Console::text;
                case Accent::primary:   break;
            }
            return Console::cyan;
        case ThemeId::ivory:
            break;
    }
    return Ivory::ink;
}

void Knob::paint (juce::Graphics& g)
{
    auto d = (float) std::min (getWidth(), getHeight());
    auto box = getLocalBounds().toFloat().withSizeKeepingCentre (d, d);
    auto p = (float) valueToProportionOfLength (getValue());
    auto angle = angleForProportion (p);
    auto from = bipolar ? angleForProportion ((float) valueToProportionOfLength (0.0)) : startAngle;

    switch (theme)
    {
        case ThemeId::graphite: paintGraphite (g, box, angle, from); break;
        case ThemeId::console:  paintConsole  (g, box, angle, from); break;
        case ThemeId::ivory:    paintIvory    (g, box, angle); break;
    }
}

void Knob::paintGraphite (juce::Graphics& g, juce::Rectangle<float> box, float angle, float from)
{
    auto d = box.getWidth();
    auto ring = d >= 80.0f ? 8.0f : 7.0f;

    fillRing (g, box, ring, startAngle, endAngle, Graphite::track);
    fillRing (g, box, ring, from, angle, accentColour());

    auto body = box.reduced (ring);
    juce::Path bodyPath;
    bodyPath.addEllipse (body);
    drawSoftShadow (g, bodyPath, juce::Colours::black.withAlpha (0.5f), d >= 80.0f ? 12.0f : 9.0f, { 0, d >= 80.0f ? 5 : 3 });
    fillRadial (g, body, 0.35f, 0.28f, 0.70f, juce::Colour (0xff4c4e55), juce::Colour (0xff1b1c1f));

    // 1px highlight along the top edge
    juce::Path rim;
    rim.addCentredArc (body.getCentreX(), body.getCentreY(), body.getWidth() * 0.5f - 0.5f, body.getHeight() * 0.5f - 0.5f,
                       0.0f, -1.1f, 1.1f, true);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.strokePath (rim, juce::PathStrokeType (1.0f));

    auto bd = body.getWidth();
    auto w = std::max (2.5f, bd * 0.034f);
    auto len = bd * 0.23f;
    auto top = bd * 0.08f;
    juce::Path pointer;
    pointer.addRoundedRectangle (-w * 0.5f, -bd * 0.5f + top, w, len, w * 0.5f);
    g.setColour (Graphite::cream);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (body.getCentre()));
}

void Knob::paintConsole (juce::Graphics& g, juce::Rectangle<float> box, float angle, float from)
{
    auto d = box.getWidth();
    auto ring = d >= 90.0f ? 5.0f : 4.0f;

    fillRing (g, box, ring, startAngle, endAngle, Console::track);
    fillRing (g, box, ring, from, angle, accentColour());

    juce::Rectangle<float> body;
    if (d >= 105.0f)
    {
        g.setColour (Console::panel);
        g.fillEllipse (box.reduced (ring));
        body = box.reduced (ring + 9.0f);
    }
    else
    {
        body = box.reduced (ring);
    }

    g.setColour (Console::inner);
    g.fillEllipse (body);

    if (d >= 105.0f)
    {
        juce::Path rim;
        rim.addCentredArc (body.getCentreX(), body.getCentreY(), body.getWidth() * 0.5f - 0.5f, body.getHeight() * 0.5f - 0.5f,
                           0.0f, -1.1f, 1.1f, true);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.strokePath (rim, juce::PathStrokeType (1.0f));
    }

    auto size = d >= 105.0f ? 17.0f : d >= 90.0f ? 15.0f : d >= 70.0f ? 12.5f : 12.0f;
    auto valueFont = fonts().get (FontId::jetMonoMedium, size);
    auto text = getDisplayText();

    if (unit.isNotEmpty() && d >= 90.0f)
    {
        auto c = body.getCentre();
        drawText (g, text, valueFont, Console::text, { body.getX(), c.y - size * 0.95f, body.getWidth(), size * 1.3f },
                  juce::Justification::centred);
        drawText (g, unit, fonts().get (FontId::interRegular, 9.5f), Console::dim,
                  { body.getX(), c.y + size * 0.42f, body.getWidth(), 12.0f }, juce::Justification::centred);
    }
    else
    {
        drawText (g, text, valueFont, Console::text, body, juce::Justification::centred);
    }
}

void Knob::paintIvory (juce::Graphics& g, juce::Rectangle<float> box, float angle)
{
    auto d = box.getWidth();
    auto c = box.getCentre();
    auto r = d * 0.5f;
    auto tick = std::max (5.0f, d * 0.072f);

    g.setColour (Ivory::ink);
    for (int i = 0; i <= 10; ++i)
    {
        auto a = startAngle + (float) i * (endAngle - startAngle) / 10.0f;
        g.drawLine ({ polar (c, r - 0.5f, a), polar (c, r - tick, a) }, 1.3f);
    }

    auto body = box.reduced ((float) juce::roundToInt (d * 0.128f));
    auto bd = body.getWidth();
    juce::Path bodyPath;
    bodyPath.addEllipse (body);
    drawSoftShadow (g, bodyPath, juce::Colour (40, 30, 20).withAlpha (0.45f), bd >= 70.0f ? 11.0f : 8.0f, { 0, bd >= 70.0f ? 5 : 4 });

    if (lighterBody)
        fillRadial (g, body, 0.40f, 0.30f, 0.69f, juce::Colour (0xff8a8174), juce::Colour (0xff3e3933));
    else
        fillRadial (g, body, 0.40f, 0.30f, 0.69f, juce::Colour (0xff3b3733), juce::Colour (0xff12100e));

    if (d >= 90.0f)
        fillRadial (g, body.reduced (bd * 0.13f), 0.38f, 0.30f, 0.65f, juce::Colour (0xff4a4540), juce::Colour (0xff1a1715));

    auto w = bd >= 70.0f ? 3.0f : 2.5f;
    auto len = bd * 0.31f;
    juce::Path pointer;
    pointer.addRoundedRectangle (-w * 0.5f, -bd * 0.5f + 3.0f, w, len, w * 0.5f);
    g.setColour (juce::Colour (0xfff4efe4));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (body.getCentre()));
}

// ============================================================
// ValueLabel
// ============================================================

ValueLabel::ValueLabel (Knob& k) : knob (k)
{
    setJustificationType (juce::Justification::centred);
    setEditable (false, true, false);
    setInterceptsMouseClicks (true, false);
    setTooltip ("Double-click to type a value");
    knob.addListener (this);
    update();
}

ValueLabel::~ValueLabel()
{
    knob.removeListener (this);
}

void ValueLabel::setStyle (const juce::Font& f, juce::Colour c)
{
    setFont (f);
    setColour (juce::Label::textColourId, c);
    setColour (juce::Label::backgroundWhenEditingColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::outlineWhenEditingColourId, c.withAlpha (0.5f));
    setColour (juce::TextEditor::textColourId, c);
    setColour (juce::TextEditor::highlightColourId, c.withAlpha (0.25f));
    setColour (juce::CaretComponent::caretColourId, c);
}

void ValueLabel::sliderValueChanged (juce::Slider*) { update(); }

void ValueLabel::update()
{
    if (! isBeingEdited())
        setText (knob.getDisplayText(), juce::dontSendNotification);
}

juce::TextEditor* ValueLabel::createEditorComponent()
{
    auto* ed = juce::Label::createEditorComponent();
    ed->setJustification (juce::Justification::centred);
    ed->setText (juce::String (knob.getValue(), 1), false);
    ed->selectAll();
    return ed;
}

void ValueLabel::textWasEdited()
{
    auto text = getText().replace (minusSign(), "-").retainCharacters ("0123456789.-+");
    if (text.isNotEmpty())
        knob.setValue (knob.getRange().clipValue (text.getDoubleValue()), juce::sendNotificationSync);
    update();
}

// ============================================================
// ThemedToggle
// ============================================================

ThemedToggle::ThemedToggle (Role r, juce::RangedAudioParameter& p, bool inv)
    : juce::Button (p.getName (64)), role (r), inverted (inv),
      attachment (p, [this] (float v)
                  {
                      bool on = v >= 0.5f;
                      setToggleState (inverted ? ! on : on, juce::dontSendNotification);
                      repaint();
                  })
{
    setClickingTogglesState (false);
    attachment.sendInitialUpdate();

    switch (role)
    {
        case Role::stageIn: setTooltip ("Stage in / out"); break;
        case Role::link:    setTooltip ("Stereo link"); break;
        case Role::bypass:  setTooltip ("Bypass the whole plugin"); break;
    }
}

void ThemedToggle::clicked()
{
    bool newState = ! getToggleState();
    attachment.setValueAsCompleteGesture ((inverted ? ! newState : newState) ? 1.0f : 0.0f);
}

void ThemedToggle::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    auto b = getLocalBounds().toFloat();
    bool on = getToggleState();

    if (theme == ThemeId::graphite)
    {
        auto pill = b.reduced (0.5f);
        juce::Path p = roundedBox (pill, pill.getHeight() * 0.5f);
        bool big = role != Role::stageIn;
        fillAndStroke (g, p, big ? Graphite::btnBg : Graphite::darkBtn,
                       highlighted ? Graphite::dim.withAlpha (0.6f) : (big ? Graphite::btnBorder : Graphite::pillBorder));

        auto lampD = big ? 8.0f : 6.0f;
        auto lampX = b.getX() + (big ? 10.0f : 7.0f) + lampD * 0.5f;
        auto lampColour = role == Role::bypass ? Graphite::bypassLamp : Graphite::lamp;
        drawLamp (g, { lampX, b.getCentreY() }, lampD, on ? lampColour : Graphite::lampOff, on);

        juce::String text = role == Role::stageIn ? (on ? "IN" : "OUT") : role == Role::link ? "LINK" : "BYPASS";
        auto font = fonts().get (FontId::interRegular, big ? 11.0f : 9.5f, 0.14f);
        auto textX = lampX + lampD * 0.5f + (big ? 8.0f : 6.0f);
        drawText (g, text, font, on ? Graphite::text : Graphite::dim,
                  { textX, b.getY(), b.getRight() - textX, b.getHeight() }, juce::Justification::centredLeft);
        return;
    }

    if (theme == ThemeId::console)
    {
        if (role == Role::stageIn)
        {
            auto pill = b.reduced (0.5f);
            auto p = roundedBox (pill, pill.getHeight() * 0.5f);
            fillAndStroke (g, p, on ? Console::cyanBg : Console::btnBg, on ? Console::cyanBorder : Console::btnBorder);
            drawText (g, on ? "ON" : "OFF", fonts().get (FontId::interSemiBold, 10.5f),
                      on ? Console::cyanSoft : Console::dim, b, juce::Justification::centred);
            return;
        }

        auto box = b.reduced (0.5f);
        auto p = roundedBox (box, 8.0f);

        if (role == Role::link)
        {
            fillAndStroke (g, p, on ? Console::cyanBg : Console::btnBg,
                           on ? Console::cyanBorder : (highlighted ? Console::dim : Console::btnBorder));
            auto colour = on ? Console::cyanSoft : Console::dim;

            // Link icon (two chain links)
            auto icon = juce::Rectangle<float> (b.getX() + 13.0f, b.getCentreY() - 7.0f, 14.0f, 14.0f);
            juce::Path chain;
            chain.addRoundedRectangle (-6.0f, -2.3f, 7.4f, 4.6f, 2.3f);
            chain.addRoundedRectangle (-1.4f, -2.3f, 7.4f, 4.6f, 2.3f);
            chain.applyTransform (juce::AffineTransform::rotation (-0.25f * pi).translated (icon.getCentre()));
            g.setColour (colour);
            g.strokePath (chain, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            drawText (g, "Link", fonts().get (FontId::interMedium, 12.0f), colour,
                      { icon.getRight() + 7.0f, b.getY(), b.getRight() - icon.getRight() - 7.0f, b.getHeight() });
            return;
        }

        // Bypass: power icon
        fillAndStroke (g, p, on ? Console::orangeBg : Console::btnBg,
                       on ? Console::orangeBorder : (highlighted ? Console::dim : Console::btnBorder));
        auto c = b.getCentre();
        juce::Path power;
        power.startNewSubPath (c.x, c.y - 5.7f);
        power.lineTo (c.x, c.y - 0.3f);
        power.addCentredArc (c.x, c.y + 0.26f, 5.0f, 5.0f, 0.0f, 0.72f, 2.0f * pi - 0.72f, true);
        g.setColour (on ? Console::orange : Console::dim);
        g.strokePath (power, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }

    // Ivory: cream rocker with an LED
    auto box = b.reduced (0.5f);
    auto radius = role == Role::stageIn ? 3.0f : 4.0f;
    auto p = roundedBox (box, radius);
    if (role == Role::stageIn)
    {
        g.setColour (Ivory::segBg);
        g.fillPath (p);
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff5f0e6), 0.0f, box.getY(),
                                                 juce::Colour (0xffddd4c3), 0.0f, box.getBottom(), false));
        g.fillPath (p);
    }
    g.setColour (highlighted ? Ivory::label : Ivory::btnBorder);
    g.strokePath (p, juce::PathStrokeType (1.0f));

    auto ledD = role == Role::stageIn ? 6.0f : 8.0f;
    drawLamp (g, b.getCentre(), ledD, on ? Ivory::ledOn : Ivory::ledOff, on);
}

// ============================================================
// ChoiceSelector
// ============================================================

ChoiceSelector::ChoiceSelector (juce::RangedAudioParameter& p, juce::StringArray s, juce::StringArray l, juce::String t)
    : param (p), shortNames (std::move (s)), longNames (std::move (l)), title (std::move (t)),
      attachment (p, [this] (float v) { index = juce::roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
    setRepaintsOnMouseActivity (false);
}

juce::Rectangle<float> ChoiceSelector::segmentBounds (int i) const
{
    auto n = shortNames.size();
    auto b = getLocalBounds().toFloat();

    if (theme == ThemeId::graphite)
    {
        const float gap = 3.0f;
        auto w = (b.getWidth() - gap * (float) (n - 1)) / (float) n;
        return { b.getX() + (float) i * (w + gap), b.getY(), w, b.getHeight() };
    }

    auto inner = b.reduced (1.0f);
    auto w = inner.getWidth() / (float) n;
    return { inner.getX() + (float) i * w, inner.getY(), w, inner.getHeight() };
}

int ChoiceSelector::segmentAt (juce::Point<float> pos) const
{
    for (int i = 0; i < shortNames.size(); ++i)
        if (segmentBounds (i).expanded (1.5f, 0.0f).contains (pos))
            return i;
    return -1;
}

void ChoiceSelector::choose (int i)
{
    if (i < 0 || i >= shortNames.size())
        return;
    attachment.setValueAsCompleteGesture ((float) i);
}

void ChoiceSelector::mouseMove (const juce::MouseEvent& e)
{
    auto h = theme == ThemeId::console ? (getLocalBounds().contains (e.getPosition()) ? 0 : -1)
                                       : segmentAt (e.position);
    if (h != hover) { hover = h; repaint(); }
}

void ChoiceSelector::mouseExit (const juce::MouseEvent&)
{
    if (hover != -1) { hover = -1; repaint(); }
}

void ChoiceSelector::mouseDown (const juce::MouseEvent& e)
{
    if (theme != ThemeId::console)
    {
        choose (segmentAt (e.position));
        return;
    }

    juce::PopupMenu menu;
    for (int i = 0; i < longNames.size(); ++i)
        menu.addItem (i + 1, longNames[i], true, i == index);

    juce::Component::SafePointer<ChoiceSelector> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                        [safe] (int result)
                        {
                            if (safe != nullptr && result > 0)
                                safe->choose (result - 1);
                        });
}

void ChoiceSelector::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    if (theme == ThemeId::console)
    {
        auto box = b.reduced (0.5f);
        auto p = roundedBox (box, 7.0f);
        fillAndStroke (g, p, Console::inner, hover == 0 ? Console::dim.withAlpha (0.6f) : Console::innerBorder);

        drawText (g, title, fonts().get (FontId::interRegular, 11.5f), Console::dim,
                  box.withTrimmedLeft (10.0f), juce::Justification::centredLeft);

        auto chev = juce::Rectangle<float> (box.getRight() - 19.0f, box.getCentreY() - 4.5f, 9.0f, 9.0f);
        g.setColour (Console::dim);
        g.strokePath (chevronPath (chev, 0), juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        auto value = spacedRatio (longNames[index]);
        drawText (g, value, fonts().get (FontId::jetMonoRegular, 11.5f), Console::text,
                  { box.getX(), box.getY(), chev.getX() - 8.0f - box.getX(), box.getHeight() },
                  juce::Justification::centredRight);
        return;
    }

    if (theme == ThemeId::graphite)
    {
        auto font = fonts().get (FontId::plexMonoMedium, 10.0f);
        for (int i = 0; i < shortNames.size(); ++i)
        {
            auto r = segmentBounds (i).reduced (0.5f);
            bool sel = i == index;
            auto p = roundedBox (r, 5.0f);
            fillAndStroke (g, p, sel ? Graphite::selBg : Graphite::darkBtn,
                           sel ? Graphite::gold : (i == hover ? Graphite::dim.withAlpha (0.7f) : Graphite::btnBorder));
            drawText (g, shortNames[i], font, sel ? Graphite::cream : Graphite::dim, r, juce::Justification::centred);
        }
        return;
    }

    // Ivory: joined segments in an ink frame
    auto frame = b.reduced (0.5f);
    auto outline = roundedBox (frame, 3.0f);
    g.saveState();
    g.reduceClipRegion (outline);
    g.setColour (Ivory::segBg);
    g.fillRect (b);

    auto font = fonts().get (FontId::plexMonoMedium, 9.5f);
    for (int i = 0; i < shortNames.size(); ++i)
    {
        auto r = segmentBounds (i);
        bool sel = i == index;
        if (sel)
        {
            g.setColour (Ivory::ink);
            g.fillRect (r);
        }
        else if (i == hover)
        {
            g.setColour (Ivory::segDivider.withAlpha (0.45f));
            g.fillRect (r);
        }

        // Divider on the right, except at the end and next to the selected one
        if (i < shortNames.size() - 1 && ! sel && i + 1 != index)
        {
            g.setColour (Ivory::segDivider);
            g.fillRect (r.getRight() - 1.0f, r.getY(), 1.0f, r.getHeight());
        }
        drawText (g, shortNames[i], font, sel ? Ivory::segSelText : Ivory::ink, r, juce::Justification::centred);
    }
    g.restoreState();

    g.setColour (Ivory::ink);
    g.strokePath (outline, juce::PathStrokeType (1.0f));
}

// ============================================================
// TransformerSelector
// ============================================================

TransformerSelector::TransformerSelector (juce::RangedAudioParameter& p)
    : param (p), attachment (p, [this] (float v) { index = juce::roundToInt (v); repaint(); })
{
    attachment.sendInitialUpdate();
}

juce::String TransformerSelector::description (int type)
{
    switch (type)
    {
        case 1: return juce::String::fromUTF8 ("Transparent, clean \xe2\x80\x94 minimal harmonics");
        case 2: return juce::String::fromUTF8 ("Warm, musical \xe2\x80\x94 rich low-order harmonics");
        case 3: return juce::String::fromUTF8 ("Aggressive, present \xe2\x80\x94 dense upper harmonics");
        default: return juce::String::fromUTF8 ("Off \xe2\x80\x94 no transformer colour");
    }
}

juce::String TransformerSelector::shortDescription (int type)
{
    switch (type)
    {
        case 1: return "Transparent, clean";
        case 2: return "Warm, musical";
        case 3: return "Aggressive, present";
        default: return "Off";
    }
}

void TransformerSelector::choose (int type)
{
    type = juce::jlimit (0, 3, type);
    if (type != index)
        attachment.setValueAsCompleteGesture ((float) type);
}

juce::Rectangle<float> TransformerSelector::itemBounds (int type) const
{
    auto b = getLocalBounds().toFloat();

    switch (theme)
    {
        case ThemeId::graphite:
        {
            const float gap = 8.0f;
            auto w = (b.getWidth() - 2.0f * gap) / 3.0f;
            return { b.getX() + (float) (type - 1) * (w + gap), b.getY(), w, 46.0f };
        }
        case ThemeId::console:
            return { b.getX(), b.getY() + (float) (type - 1) * 66.0f, b.getWidth(), 58.0f };
        case ThemeId::ivory:
            break;
    }

    // Ivory: label boxes around the rotary switch
    switch (type)
    {
        case 0:  return { 0.0f, 62.0f, 30.0f, 13.0f };   // OFF
        case 1:  return { 0.0f, 20.0f, 54.0f, 13.0f };   // NICKEL
        case 2:  return { 59.0f, 2.0f, 32.0f, 13.0f };   // IRON
        default: return { 112.0f, 20.0f, 38.0f, 13.0f }; // STEEL
    }
}

juce::Point<float> TransformerSelector::ivoryKnobCentre() const { return { 75.0f, 54.0f }; }

float TransformerSelector::ivoryAngle (int type) const
{
    const float degrees[] = { -100.0f, -40.0f, 0.0f, 40.0f };
    return juce::degreesToRadians (degrees[juce::jlimit (0, 3, type)]);
}

void TransformerSelector::mouseDown (const juce::MouseEvent& e)
{
    dragStartIndex = index;

    if (theme == ThemeId::ivory)
    {
        for (int t = 0; t <= 3; ++t)
            if (itemBounds (t).expanded (3.0f).contains (e.position))
                { choose (t); return; }

        if (e.position.getDistanceFrom (ivoryKnobCentre()) <= 32.0f)
            choose ((index + 1) % 4);   // click on the switch: next position
        return;
    }

    for (int t = 1; t <= 3; ++t)
        if (itemBounds (t).contains (e.position))
        {
            choose (index == t ? 0 : t);   // clicking the selected type turns it off
            return;
        }
}

void TransformerSelector::mouseDrag (const juce::MouseEvent& e)
{
    if (theme != ThemeId::ivory || e.mouseDownPosition.getDistanceFrom (ivoryKnobCentre()) > 32.0f)
        return;

    auto steps = (e.getDistanceFromDragStartX() - e.getDistanceFromDragStartY()) / 24;
    choose (dragStartIndex + steps);
}

void TransformerSelector::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (theme != ThemeId::ivory)
    {
        Component::mouseWheelMove (e, w);
        return;
    }
    auto delta = w.deltaY != 0.0f ? w.deltaY : w.deltaX;
    if (delta > 0.0f) choose (index + 1);
    else if (delta < 0.0f) choose (index - 1);
}

void TransformerSelector::paint (juce::Graphics& g)
{
    const juce::StringArray names { "OFF", "NICKEL", "IRON", "STEEL" };

    if (theme == ThemeId::graphite)
    {
        auto font = fonts().get (FontId::interRegular, 11.0f, 0.18f);
        for (int t = 1; t <= 3; ++t)
        {
            auto r = itemBounds (t).reduced (0.5f);
            bool sel = t == index;
            auto p = roundedBox (r, 8.0f);
            fillAndStroke (g, p, sel ? Graphite::selBg : Graphite::darkBtn, sel ? Graphite::gold : Graphite::btnBorder);

            auto bar = juce::Rectangle<float> (r.getCentreX() - 9.0f, r.getY() + 12.5f, 18.0f, 3.0f);
            if (sel)
            {
                g.setColour (Graphite::lamp.withAlpha (0.25f));
                g.fillRoundedRectangle (bar.expanded (2.5f), 3.0f);
            }
            g.setColour (sel ? Graphite::lamp : Graphite::btnBorder);
            g.fillRoundedRectangle (bar, 1.5f);

            // tracking adds space after the last letter too; shift to keep it centred
            drawText (g, names[t], font, sel ? Graphite::cream : Graphite::dim,
                      { r.getX() + 1.0f, r.getY() + 19.0f, r.getWidth(), 15.0f }, juce::Justification::centred);
        }

        drawText (g, description (index), fonts().get (FontId::interRegular, 10.5f), Graphite::dim,
                  { 0.0f, (float) getHeight() - 14.0f, (float) getWidth(), 14.0f });
        return;
    }

    if (theme == ThemeId::console)
    {
        const char* titles[] = { "Off", "Nickel", "Iron", "Steel" };
        for (int t = 1; t <= 3; ++t)
        {
            auto r = itemBounds (t).reduced (0.5f);
            bool sel = t == index;
            auto p = roundedBox (r, 9.0f);
            fillAndStroke (g, p, sel ? Console::cyanBg : Console::inner, sel ? Console::cyan : Console::innerBorder);
            drawText (g, titles[t], fonts().get (FontId::interSemiBold, 12.5f), sel ? Console::text : Console::soft,
                      { r.getX() + 12.0f, r.getY() + 13.0f, r.getWidth() - 24.0f, 16.0f });
            drawText (g, shortDescription (t), fonts().get (FontId::interRegular, 10.0f), sel ? Console::cyanSoft : Console::dim,
                      { r.getX() + 12.0f, r.getY() + 31.0f, r.getWidth() - 24.0f, 13.0f });
        }
        return;
    }

    // Ivory rotary switch
    auto font = fonts().get (FontId::plexCondRegular, 10.0f, 0.16f);
    auto selFont = fonts().get (FontId::plexCondSemiBold, 10.0f, 0.16f);
    for (int t = 0; t <= 3; ++t)
    {
        auto r = itemBounds (t);
        bool sel = t == index;
        auto just = t == 3 ? juce::Justification::centredRight : juce::Justification::centredLeft;
        drawText (g, names[t], sel ? selFont : font, sel ? Ivory::red : Ivory::label, r, just);
    }

    auto c = ivoryKnobCentre();
    auto body = juce::Rectangle<float> (60.0f, 60.0f).withCentre (c);
    juce::Path bodyPath;
    bodyPath.addEllipse (body);
    drawSoftShadow (g, bodyPath, juce::Colour (40, 30, 20).withAlpha (0.45f), 8.0f, { 0, 4 });
    fillRadial (g, body, 0.40f, 0.30f, 0.69f, juce::Colour (0xff3b3733), juce::Colour (0xff12100e));

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -27.0f, 3.0f, 16.0f, 1.5f);
    g.setColour (juce::Colour (0xfff4efe4));
    g.fillPath (pointer, juce::AffineTransform::rotation (ivoryAngle (index)).translated (c));
}

// ============================================================
// HeaderButton
// ============================================================

HeaderButton::HeaderButton (Kind k) : juce::Button ({}), kind (k)
{
    switch (kind)
    {
        case Kind::previous:   setTooltip ("Previous preset"); break;
        case Kind::next:       setTooltip ("Next preset"); break;
        case Kind::presetName: setTooltip ("Presets"); break;
        case Kind::save:       setTooltip ("Save the current settings as a preset"); break;
        case Kind::compareA:   setTooltip ("Compare: setting A"); break;
        case Kind::compareB:   setTooltip ("Compare: setting B"); break;
    }
}

void HeaderButton::setPreset (const juce::String& name, bool isModified, int n)
{
    if (name == presetName && isModified == modified && n == number)
        return;
    presetName = name;
    modified = isModified;
    number = n;
    repaint();
}

void HeaderButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto b = getLocalBounds().toFloat();
    auto box = b.reduced (0.5f);
    auto shownName = presetName.isEmpty() ? juce::String ("Untitled") : presetName;

    if (theme == ThemeId::graphite)
    {
        auto border = highlighted ? Graphite::dim.withAlpha (0.7f) : Graphite::btnBorder;
        switch (kind)
        {
            case Kind::previous:
            case Kind::next:
            {
                fillAndStroke (g, roundedBox (box, 6.0f), down ? Graphite::btnBorder : Graphite::btnBg, border);
                g.setColour (Graphite::chevron);
                g.strokePath (chevronPath (b.withSizeKeepingCentre (12.0f, 12.0f), kind == Kind::previous ? -1 : 1),
                              juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                break;
            }
            case Kind::presetName:
            {
                fillAndStroke (g, roundedBox (box, 6.0f), Graphite::inset, border);
                auto chev = juce::Rectangle<float> (b.getRight() - 22.0f, b.getCentreY() - 5.0f, 10.0f, 10.0f);
                g.setColour (Graphite::dim);
                g.strokePath (chevronPath (chev, 0), juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                auto textArea = juce::Rectangle<float> (b.getX() + 12.0f, b.getY(), chev.getX() - b.getX() - 18.0f, b.getHeight());
                if (modified)
                {
                    drawLamp (g, { chev.getX() - 6.0f, b.getCentreY() }, 5.0f, Graphite::gold, false);
                    textArea.removeFromRight (8.0f);
                }
                g.setFont (fonts().get (FontId::interRegular, 12.5f));
                g.setColour (Graphite::text);
                g.drawText (shownName, textArea, juce::Justification::centredLeft, true);
                break;
            }
            case Kind::save:
                fillAndStroke (g, roundedBox (box, 6.0f), down ? Graphite::saveBorder : Graphite::selBg,
                               highlighted ? Graphite::gold : Graphite::saveBorder);
                drawText (g, "Save", fonts().get (FontId::interMedium, 12.0f), Graphite::goldLight, b, juce::Justification::centred);
                break;
            case Kind::compareA:
            case Kind::compareB:
                break;
        }
        return;
    }

    if (theme == ThemeId::console)
    {
        auto border = highlighted ? Console::dim.withAlpha (0.7f) : Console::btnBorder;
        switch (kind)
        {
            case Kind::previous:
            case Kind::next:
                fillAndStroke (g, roundedBox (box, 8.0f), down ? Console::abActive : Console::btnBg, border);
                g.setColour (Console::soft);
                g.strokePath (chevronPath (b.withSizeKeepingCentre (12.0f, 12.0f), kind == Kind::previous ? -1 : 1),
                              juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                break;
            case Kind::presetName:
            {
                fillAndStroke (g, roundedBox (box, 8.0f), Console::btnBg, border);
                // list icon
                auto ix = b.getX() + 13.0f, iy = b.getCentreY() - 7.0f;
                juce::Path lines;
                lines.startNewSubPath (ix + 2.0f, iy + 3.5f);  lines.lineTo (ix + 12.0f, iy + 3.5f);
                lines.startNewSubPath (ix + 2.0f, iy + 7.0f);  lines.lineTo (ix + 12.0f, iy + 7.0f);
                lines.startNewSubPath (ix + 2.0f, iy + 10.5f); lines.lineTo (ix + 8.0f, iy + 10.5f);
                g.setColour (Console::dim);
                g.strokePath (lines, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

                auto textArea = juce::Rectangle<float> (b.getX() + 37.0f, b.getY(), b.getWidth() - 37.0f - 32.0f, b.getHeight());
                g.setFont (fonts().get (FontId::interRegular, 12.5f));
                g.setColour (Console::text);
                g.drawText (shownName, textArea, juce::Justification::centredLeft, true);
                if (modified)
                    drawLamp (g, { b.getRight() - 17.0f, b.getCentreY() }, 7.0f, Console::cyan, false);
                break;
            }
            case Kind::save:
                g.setColour (down ? Console::cyanSoft : (highlighted ? Console::cyan.brighter (0.1f) : Console::cyan));
                g.fillRoundedRectangle (b, 8.0f);
                drawText (g, "Save", fonts().get (FontId::interSemiBold, 12.0f), Console::saveText, b, juce::Justification::centred);
                break;
            case Kind::compareA:
            case Kind::compareB:
            {
                bool left = kind == Kind::compareA;
                auto p = roundedBox (box, 8.0f, left, ! left);
                fillAndStroke (g, p, active ? Console::abActive : Console::btnBg, Console::btnBorder);
                drawText (g, left ? "A" : "B", fonts().get (FontId::interSemiBold, 12.0f),
                          active ? Console::text : (highlighted ? Console::soft : Console::dim), b, juce::Justification::centred);
                break;
            }
        }
        return;
    }

    // Ivory
    switch (kind)
    {
        case Kind::previous:
        case Kind::next:
            g.setColour (down ? Ivory::segDivider : Ivory::btnBg);
            g.fillEllipse (box);
            g.setColour (highlighted ? Ivory::label : Ivory::btnBorder);
            g.drawEllipse (box, 1.0f);
            g.setColour (Ivory::ink);
            g.strokePath (chevronPath (b.withSizeKeepingCentre (11.0f, 11.0f), kind == Kind::previous ? -1 : 1),
                          juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            break;
        case Kind::presetName:
        {
            g.setColour (Ivory::display);
            g.fillRoundedRectangle (b, 4.0f);
            // inset shadow along the top
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.6f), 0.0f, b.getY() + 2.0f,
                                                     juce::Colours::transparentBlack, 0.0f, b.getY() + 9.0f, false));
            g.fillRoundedRectangle (b.reduced (2.0f), 3.0f);
            g.setColour (Ivory::displayEdge);
            g.drawRoundedRectangle (b.reduced (1.0f), 4.0f, 2.0f);

            auto numberText = number > 0 ? juce::String (number).paddedLeft ('0', 2) : juce::String ("--");
            drawText (g, numberText, fonts().get (FontId::plexMonoMedium, 10.0f), Ivory::displayDim,
                      b.withTrimmedRight (12.0f), juce::Justification::centredRight);
            g.setFont (fonts().get (FontId::plexMonoMedium, 12.0f));
            g.setColour (Ivory::displayText);
            g.drawText (modified ? shownName + "*" : shownName,
                        b.withTrimmedLeft (12.0f).withTrimmedRight (40.0f), juce::Justification::centredLeft, true);
            break;
        }
        case Kind::save:
            g.setColour (down ? Ivory::label : (highlighted ? Ivory::ink.brighter (0.15f) : Ivory::ink));
            g.fillRoundedRectangle (b, 4.0f);
            g.setColour (Ivory::displayEdge);
            g.drawRoundedRectangle (box, 4.0f, 1.0f);
            drawText (g, "SAVE", fonts().get (FontId::plexCondRegular, 11.0f, 0.18f), Ivory::face,
                      b.withTrimmedLeft (2.0f), juce::Justification::centred);
            break;
        case Kind::compareA:
        case Kind::compareB:
            break;
    }
}

// ============================================================
// ThemeSwatch
// ============================================================

ThemeSwatch::ThemeSwatch (ThemeId r) : juce::Button ({}), represents (r)
{
    const char* names[] = { "Graphite & Brass", "Mastering Console", "Ivory Rack" };
    setTooltip (juce::String ("Theme: ") + names[(int) r]);
}

void ThemeSwatch::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    auto b = getLocalBounds().toFloat().withSizeKeepingCentre (20.0f, 20.0f);

    juce::Colour c1, c2;
    switch (represents)
    {
        case ThemeId::graphite: c1 = juce::Colour (0xff2a2b30); c2 = Graphite::gold; break;
        case ThemeId::console:  c1 = Console::bg;               c2 = Console::cyan;  break;
        case ThemeId::ivory:    c1 = Ivory::face;               c2 = Ivory::red;     break;
    }

    juce::Colour activeEdge, idleEdge;
    switch (theme)
    {
        case ThemeId::graphite: activeEdge = Graphite::gold; idleEdge = Graphite::btnBorder; break;
        case ThemeId::console:  activeEdge = Console::cyan;  idleEdge = Console::btnBorder;  break;
        case ThemeId::ivory:    activeEdge = Ivory::red;     idleEdge = Ivory::swatchEdge;   break;
    }

    auto inner = b.reduced (2.0f);
    juce::Path circle;
    circle.addEllipse (inner);
    g.saveState();
    g.reduceClipRegion (circle);
    g.setColour (c1);
    g.fillRect (inner);
    juce::Path half;
    half.addTriangle (inner.getTopRight(), inner.getBottomRight(), inner.getBottomLeft());
    g.setColour (c2);
    g.fillPath (half);
    g.restoreState();

    bool selected = represents == theme;
    g.setColour (selected ? activeEdge : (highlighted ? activeEdge.withAlpha (0.5f) : idleEdge));
    g.drawEllipse (b.reduced (1.0f), 2.0f);
}
