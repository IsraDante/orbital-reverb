#include "PluginEditor.h"

//==============================================================================
namespace theme
{
    juce::Font mono (float px, float letterSpacingEm)
    {
        return juce::Font (juce::FontOptions()
                               .withName (juce::Font::getDefaultMonospacedFontName())
                               .withPointHeight (px)
                               .withKerningFactor (letterSpacingEm));
    }

    juce::Font serifItalic (float px, float letterSpacingEm)
    {
        return juce::Font (juce::FontOptions()
                               .withName (juce::Font::getDefaultSerifFontName())
                               .withStyle ("Italic")
                               .withPointHeight (px)
                               .withKerningFactor (letterSpacingEm));
    }
}

namespace
{
    float textWidth (const juce::Font& font, const juce::String& text)
    {
        return juce::GlyphArrangement::getStringWidth (font, text);
    }

    // Igual que la funcion arc() del HTML: angulos en grados, 0 = arriba, sentido horario
    juce::Path arcPath (juce::Point<float> centre, float radius, float fromDeg, float toDeg)
    {
        juce::Path p;
        p.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                         juce::degreesToRadians (fromDeg), juce::degreesToRadians (toDeg), true);
        return p;
    }

    void strokeDashed (juce::Graphics& g, const juce::Path& path, float width, std::initializer_list<float> dashes)
    {
        std::vector<float> d (dashes);
        juce::Path dashed;
        juce::PathStrokeType (width).createDashedStroke (dashed, path, d.data(), (int) d.size());
        g.fillPath (dashed);
    }
}

//==============================================================================
void OrbitalLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float, float, juce::Slider&)
{
    const float side = (float) juce::jmin (width, height);
    const float s = side / 100.0f; // el knob se dibuja en una caja de 100x100 como el SVG
    const juce::Point<float> c ((float) x + (float) width * 0.5f, (float) y + (float) height * 0.5f);

    const juce::PathStrokeType roundStroke (2.2f * s, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    g.setColour (theme::track);
    g.strokePath (arcPath (c, 42.0f * s, -135.0f, 135.0f), juce::PathStrokeType (1.2f * s));

    g.setColour (theme::accent);
    g.strokePath (arcPath (c, 42.0f * s, -135.0f, -135.0f + 270.0f * juce::jmax (sliderPos, 0.004f)), roundStroke);

    g.setColour (theme::track);
    g.drawEllipse (c.x - 30.0f * s, c.y - 30.0f * s, 60.0f * s, 60.0f * s, 1.0f * s);

    const float angle = juce::degreesToRadians (-135.0f + 270.0f * sliderPos);
    const juce::Point<float> dot (c.x + 24.0f * s * std::sin (angle), c.y - 24.0f * s * std::cos (angle));
    g.setColour (theme::accent);
    g.fillEllipse (dot.x - 3.5f * s, dot.y - 3.5f * s, 7.0f * s, 7.0f * s);
}

//==============================================================================
OrbitalKnob::OrbitalKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramID,
                          const juce::String& labelText, int diameterPx)
    : label (labelText), diameter (diameterPx)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity (170); // como el HTML: 170 px = recorrido completo
    slider.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    slider.setPopupDisplayEnabled (false, false, nullptr);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramID, slider);

    if (auto* param = state.getParameter (paramID))
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));

    slider.onValueChange = [this] { repaint(); };
}

void OrbitalKnob::resized()
{
    slider.setBounds (0, 0, diameter, diameter);
}

void OrbitalKnob::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();

    g.setColour (theme::text.withAlpha (0.45f));
    g.setFont (theme::mono (8.5f, 0.25f));
    g.drawText (label.toUpperCase(), juce::Rectangle<float> (-20.0f, (float) diameter + 2.0f, w + 40.0f, 11.0f),
                juce::Justification::centred, false);

    g.setColour (theme::text);
    g.setFont (theme::mono (12.0f));
    g.drawText (slider.getTextFromValue (slider.getValue()),
                juce::Rectangle<float> (-20.0f, (float) diameter + 15.0f, w + 40.0f, 15.0f),
                juce::Justification::centred, false);
}

//==============================================================================
void BypassButton::paintButton (juce::Graphics& g, bool isHighlighted, bool)
{
    const bool bypassed = getToggleState();
    const auto font = theme::mono (10.0f, 0.2f);
    const juce::String label = bypassed ? "BYPASS" : "ACTIVE";

    const float glyph = 7.0f, gap = 7.0f;
    const float total = glyph + gap + textWidth (font, label);
    const float x0 = ((float) getWidth() - total) * 0.5f;
    const float cy = (float) getHeight() * 0.5f;

    g.setColour (theme::text.withAlpha (isHighlighted ? 1.0f : 0.92f));
    if (bypassed)
        g.drawEllipse (x0 + 0.5f, cy - glyph * 0.5f + 0.5f, glyph - 1.0f, glyph - 1.0f, 1.0f);
    else
        g.fillEllipse (x0, cy - glyph * 0.5f, glyph, glyph);

    g.setFont (font);
    g.drawText (label, juce::Rectangle<float> (x0 + glyph + gap, 0.0f, (float) getWidth(), (float) getHeight()),
                juce::Justification::centredLeft, false);

    g.setColour (bypassed ? theme::buttonLine : theme::accent);
    g.fillRect (0.0f, (float) getHeight() - 1.0f, (float) getWidth(), 1.0f);
}

//==============================================================================
OrbitalPanel::OrbitalPanel (OrbitalReverbProcessor& p)
{
    setLookAndFeel (&lookAndFeel);

    auto add = [this, &p] (const char* id, const char* label, int diameter)
    {
        knobs.push_back (std::make_unique<OrbitalKnob> (p.apvts, id, label, diameter));
        addAndMakeVisible (*knobs.back());
        return knobs.back().get();
    };

    groups.push_back ({ "SPACE", 0, { add ("planet", "Planet", 112), add ("size", "Size", 84), add ("predelay", "Pre-delay", 84) } });
    groups.push_back ({ "TAIL",  0, { add ("tone", "Tone", 89), add ("decay", "Decay", 89) } });
    groups.push_back ({ "MIX",   0, { add ("mix", "Mix", 94), add ("output", "Output", 94) } });

    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "bypass", bypassButton);
    bypassButton.onStateChange = [this] { repaint (titleArea); };

    setSize (baseWidth, baseHeight);
}

OrbitalPanel::~OrbitalPanel()
{
    setLookAndFeel (nullptr);
}

void OrbitalPanel::layoutGroup (Group& group, int top)
{
    group.top = top;
    const int gap = 24;
    int total = -gap, tallest = 0;
    for (auto* k : group.knobs)
    {
        total += k->getDiameter() + gap;
        tallest = juce::jmax (tallest, k->getTotalHeight());
    }

    int x = (getWidth() - total) / 2;
    const int rowTop = top + 32;
    for (auto* k : group.knobs)
    {
        k->setBounds (x, rowTop + (tallest - k->getTotalHeight()) / 2, k->getDiameter(), k->getTotalHeight());
        x += k->getDiameter() + gap;
    }
}

void OrbitalPanel::resized()
{
    titleArea = { 32, 44, baseWidth - 64, 16 };

    layoutGroup (groups[0], 68);
    layoutGroup (groups[1], 252);
    layoutGroup (groups[2], 413);

    meterArea = { 32, 585, baseWidth - 64, 16 };
    bypassButton.setBounds ((baseWidth - 120) / 2, 621, 120, 26);
}

void OrbitalPanel::setLevel (float newLevel)
{
    if (std::abs (newLevel - level) < 0.002f)
        return;
    level = newLevel;
    repaint (titleArea);
    repaint (meterArea);
}

void OrbitalPanel::paintOrbits (juce::Graphics& g, juce::Rectangle<float> box)
{
    // Dibujo orbital del fondo (el SVG del HTML, 420x640, modo "slice")
    const float sc = juce::jmax (box.getWidth() / 420.0f, box.getHeight() / 640.0f);
    const auto t = juce::AffineTransform::scale (sc)
                       .translated (box.getX() + (box.getWidth() - 420.0f * sc) * 0.5f,
                                    box.getY() + (box.getHeight() - 640.0f * sc) * 0.5f);

    auto circle = [&] (float cx, float cy, float r)
    {
        juce::Path p;
        p.addEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        p.applyTransform (t);
        return p;
    };

    auto ellipse = [&] (float rx, float ry)
    {
        juce::Path p;
        p.addEllipse (210.0f - rx, 325.0f - ry, rx * 2.0f, ry * 2.0f);
        p.applyTransform (juce::AffineTransform::rotation (juce::degreesToRadians (-14.0f), 210.0f, 325.0f).followedBy (t));
        return p;
    };

    const float w = 0.8f * sc;
    g.setColour (theme::orbit);

    g.strokePath (circle (210, 190, 70), juce::PathStrokeType (w));
    strokeDashed (g, circle (210, 190, 115), w, { 2 * sc, 5 * sc });
    g.strokePath (circle (210, 190, 165), juce::PathStrokeType (w));
    strokeDashed (g, circle (210, 190, 215), w, { 1 * sc, 7 * sc });
    strokeDashed (g, circle (210, 460, 60), w, { 2 * sc, 5 * sc });
    g.strokePath (circle (210, 460, 110), juce::PathStrokeType (w));
    strokeDashed (g, circle (210, 460, 160), w, { 1 * sc, 7 * sc });
    g.strokePath (ellipse (230, 55), juce::PathStrokeType (w));
    strokeDashed (g, ellipse (190, 38), w, { 3 * sc, 6 * sc });

    g.setColour (theme::star);
    for (auto [x, y, r] : { std::tuple { 325.0f, 190.0f, 3.0f }, { 102.0f, 140.0f, 2.0f }, { 168.0f, 570.0f, 3.0f },
                            { 360.0f, 400.0f, 2.0f }, { 52.0f, 300.0f, 2.5f }, { 395.0f, 250.0f, 1.6f } })
        g.fillPath (circle (x, y, r));
}

void OrbitalPanel::paintFrame (juce::Graphics& g, juce::Rectangle<float> box)
{
    // Halo exterior
    for (int i = 1; i <= 10; ++i)
    {
        g.setColour (theme::accent.withAlpha (0.035f * (1.0f - (float) i / 11.0f)));
        g.drawRoundedRectangle (box.expanded ((float) i * 1.6f), 28.0f + (float) i * 1.6f, 1.8f);
    }

    // Brillo interior
    for (int i = 1; i <= 12; ++i)
    {
        g.setColour (theme::accent.withAlpha (0.012f * (1.0f - (float) i / 13.0f)));
        g.drawRoundedRectangle (box.reduced ((float) i * 2.2f), juce::jmax (4.0f, 28.0f - (float) i * 2.2f), 2.4f);
    }

    g.setColour (theme::accent);
    g.drawRoundedRectangle (box.reduced (0.75f), 27.25f, 1.5f);

    g.setColour (theme::accent.withAlpha (0.28f));
    g.drawRoundedRectangle (box.reduced (7.5f), 22.0f, 1.0f);

    g.setColour (theme::accent);
    for (auto corner : { box.getTopLeft() + juce::Point<float> (16, 16),
                         box.getTopRight() + juce::Point<float> (-16, 16),
                         box.getBottomLeft() + juce::Point<float> (16, -16),
                         box.getBottomRight() + juce::Point<float> (-16, -16) })
        g.fillEllipse (corner.x - 2.3f, corner.y - 2.3f, 4.6f, 4.6f);
}

void OrbitalPanel::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);

    const auto box = getLocalBounds().toFloat().reduced (10.0f);

    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip;
        clip.addRoundedRectangle (box, 28.0f);
        g.reduceClipRegion (clip);
        paintOrbits (g, box);
    }

    paintFrame (g, box);

    const bool bypassed = bypassButton.getToggleState();
    const float lv = juce::jlimit (0.0f, 1.0f, level * 2.0f);

    // Titulo + LED
    {
        const auto font = theme::mono (11.0f, 0.6f);
        const juce::String title ("ORBITAL REVERB");
        const float tw = textWidth (font, title);
        const float led = 9.0f, gap = 12.0f;
        const float x0 = (float) titleArea.getCentreX() - (tw + gap + led) * 0.5f;
        const float cy = (float) titleArea.getCentreY();

        g.setColour (theme::text.withAlpha (0.7f));
        g.setFont (font);
        g.drawText (title, juce::Rectangle<float> (x0, (float) titleArea.getY(), tw + 20.0f, (float) titleArea.getHeight()),
                    juce::Justification::centredLeft, false);

        const juce::Point<float> c (x0 + tw + gap + led * 0.5f, cy);
        if (bypassed)
        {
            g.setColour (theme::ledOff);
        }
        else
        {
            const float glow = 6.0f + 12.0f * lv;
            juce::ColourGradient halo (theme::accent.withAlpha (0.25f + 0.55f * lv), c, theme::accent.withAlpha (0.0f),
                                       c.translated (led * 0.5f + glow, 0.0f), true);
            g.setGradientFill (halo);
            g.fillEllipse (c.x - led * 0.5f - glow, c.y - led * 0.5f - glow, led + glow * 2.0f, led + glow * 2.0f);
            g.setColour (theme::accent.withAlpha (0.75f + 0.25f * lv));
        }
        g.fillEllipse (c.x - led * 0.5f, c.y - led * 0.5f, led, led);
    }

    // Etiquetas de grupo
    g.setFont (theme::mono (9.0f, 0.5f));
    g.setColour (theme::text.withAlpha (0.5f));
    for (auto& group : groups)
        g.drawText (group.title, juce::Rectangle<int> (0, group.top + 5, getWidth(), 12), juce::Justification::centred, false);

    // Medidor OUT
    {
        const auto font = theme::mono (11.0f);
        g.setFont (font);
        g.setColour (theme::text);
        const float labelW = textWidth (font, "OUT");
        g.drawText ("OUT", meterArea.toFloat(), juce::Justification::centredLeft, false);

        auto bar = meterArea.toFloat().withTrimmedLeft (labelW + 8.0f);
        bar = bar.withSizeKeepingCentre (bar.getWidth(), 4.0f);
        g.setColour (theme::meterBack);
        g.fillRect (bar);
        g.setColour (theme::accent);
        g.fillRect (bar.withWidth (bar.getWidth() * juce::jmin (1.0f, level * 1.6f)));
    }

    // Firma
    g.setFont (theme::serifItalic (12.0f, 0.14f));
    g.setColour (theme::accent.withAlpha (0.75f));
    g.drawText ("by Dante Ramos", juce::Rectangle<float> (box.getX(), box.getBottom() - 30.0f, box.getWidth(), 18.0f),
                juce::Justification::centred, false);
}

//==============================================================================
OrbitalReverbEditor::OrbitalReverbEditor (OrbitalReverbProcessor& p)
    : AudioProcessorEditor (&p), processor (p), panel (p)
{
    addAndMakeVisible (panel);

    setResizable (true, true);
    setResizeLimits (OrbitalPanel::baseWidth * 3 / 4, OrbitalPanel::baseHeight * 3 / 4,
                     OrbitalPanel::baseWidth * 2,     OrbitalPanel::baseHeight * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) OrbitalPanel::baseWidth / (double) OrbitalPanel::baseHeight);

    setSize (OrbitalPanel::baseWidth, OrbitalPanel::baseHeight);
    startTimerHz (30);
}

OrbitalReverbEditor::~OrbitalReverbEditor()
{
    stopTimer();
}

void OrbitalReverbEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);
}

void OrbitalReverbEditor::resized()
{
    const float scale = (float) getWidth() / (float) OrbitalPanel::baseWidth;
    panel.setBounds (0, 0, OrbitalPanel::baseWidth, OrbitalPanel::baseHeight);
    panel.setTransform (juce::AffineTransform::scale (scale));
}

void OrbitalReverbEditor::timerCallback()
{
    panel.setLevel (processor.outputPeak.exchange (0.0f));
}
