#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

//==============================================================================
namespace ui
{
    const juce::Colour cream    { 0xffeadcb8 };
    const juce::Colour lcdGreen { 0xffb9f27c };
    const juce::Colour lcdDim   { 0xff5f6d4c };
    const juce::Colour lcdBg    { 0xff101309 };
    const juce::Colour ink      { 0xff26272a };

    juce::Font makeFont (float size, int style = juce::Font::plain)
    {
        return juce::Font (juce::FontOptions (size, style));
    }

    juce::Font makeMono (float size)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::bold));
    }

    juce::ColourGradient chromeVertical (juce::Rectangle<float> r)
    {
        juce::ColourGradient grad (juce::Colour (0xfff6f7f9), 0.0f, r.getY(),
                                   juce::Colour (0xff595d63), 0.0f, r.getBottom(), false);
        grad.addColour (0.45, juce::Colour (0xffb4b8be));
        grad.addColour (0.55, juce::Colour (0xff8d9198));
        return grad;
    }

    // Text with a small dark "engraved" shadow, for use on the wood.
    void drawEngraved (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                       float size, juce::Justification just = juce::Justification::centredTop)
    {
        g.setFont (makeFont (size, juce::Font::bold));
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.drawFittedText (text, area.translated (1, 1), just, 2);
        g.setColour (cream);
        g.drawFittedText (text, area, just, 2);
    }

    // A chrome knob with a knurled rim and a dark pointer. pointerAngle: 0 = 12 o'clock, clockwise.
    void drawChromeKnob (juce::Graphics& g, juce::Point<float> c, float r, float pointerAngle)
    {
        // Shadow
        {
            juce::Path p;
            p.addEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
            juce::DropShadow (juce::Colours::black.withAlpha (0.7f), (int) (r * 0.35f),
                              juce::Point<int> (0, (int) (r * 0.12f))).drawForPath (g, p);
        }

        // Rim
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4f5f7), c.x - r * 0.7f, c.y - r * 0.7f,
                                                 juce::Colour (0xff5d6066), c.x + r * 0.7f, c.y + r * 0.7f, false));
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

        // Knurling
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        const int teeth = 64;
        for (int i = 0; i < teeth; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / (float) teeth;
            const float s = std::sin (a), co = std::cos (a);
            g.drawLine (c.x + s * r * 0.90f, c.y - co * r * 0.90f,
                        c.x + s * r * 0.995f, c.y - co * r * 0.995f, 1.0f);
        }

        // Bevel groove
        const float r2 = r * 0.86f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3e4146), c.x - r2 * 0.7f, c.y - r2 * 0.7f,
                                                 juce::Colour (0xffdadde1), c.x + r2 * 0.7f, c.y + r2 * 0.7f, false));
        g.fillEllipse (c.x - r2, c.y - r2, r2 * 2.0f, r2 * 2.0f);

        // Face
        const float r3 = r * 0.80f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffbfbfc), c.x - r3 * 0.35f, c.y - r3 * 0.45f,
                                                 juce::Colour (0xff81858b), c.x + r3 * 0.85f, c.y + r3 * 0.9f, true));
        g.fillEllipse (c.x - r3, c.y - r3, r3 * 2.0f, r3 * 2.0f);

        // Faint brushed rings
        for (int i = 1; i <= 9; ++i)
        {
            const float rr = r3 * (float) i / 10.0f;
            g.setColour ((i % 2 == 0 ? juce::Colours::white : juce::Colours::black).withAlpha (0.05f));
            g.drawEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f, 1.0f);
        }

        // Pointer
        const float s = std::sin (pointerAngle), co = std::cos (pointerAngle);
        juce::Path pointer;
        pointer.startNewSubPath (c.x + s * r3 * 0.30f, c.y - co * r3 * 0.30f);
        pointer.lineTo (c.x + s * r3 * 0.92f, c.y - co * r3 * 0.92f);

        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.strokePath (pointer, juce::PathStrokeType (r * 0.11f + 1.5f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xff17181a));
        g.strokePath (pointer, juce::PathStrokeType (r * 0.10f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }
}

//==============================================================================
// Look and feel

juce::Font HarmonioLookAndFeel::getLabelFont (juce::Label&)
{
    return ui::makeMono (15.0f);
}

void HarmonioLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    const auto centre = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).getCentre();
    const float r = juce::jmin ((float) width, (float) height) * 0.5f - 14.0f;

    if (r < 8.0f)
        return;

    // Scale ticks around the knob
    const int numTicks = 11;
    for (int i = 0; i < numTicks; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / (float) (numTicks - 1);
        const float len = (i % 5 == 0) ? 8.0f : 5.0f;
        const float s = std::sin (a), c = std::cos (a);

        g.setColour (ui::cream.withAlpha (0.8f));
        g.drawLine (centre.x + s * (r + 4.0f), centre.y - c * (r + 4.0f),
                    centre.x + s * (r + 4.0f + len), centre.y - c * (r + 4.0f + len), 1.4f);
    }

    ui::drawChromeKnob (g, centre, r, startAngle + sliderPos * (endAngle - startAngle));
}

void HarmonioLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool, bool)
{
    auto r = button.getLocalBounds().toFloat().reduced (2.0f);
    const float corner = r.getHeight() * 0.28f;

    // Shadow
    {
        juce::Path p;
        p.addRoundedRectangle (r, corner);
        juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 6, juce::Point<int> (0, 2)).drawForPath (g, p);
    }

    // Chrome frame
    g.setGradientFill (ui::chromeVertical (r));
    g.fillRoundedRectangle (r, corner);

    // Dark slot
    auto slot = r.reduced (4.0f);
    g.setColour (juce::Colour (0xff0b0c0d));
    g.fillRoundedRectangle (slot, corner * 0.7f);

    // Lever
    const bool on = button.getToggleState();
    const float lw = slot.getWidth() * 0.46f;
    juce::Rectangle<float> lever (on ? slot.getRight() - lw : slot.getX(), slot.getY(), lw, slot.getHeight());
    lever = lever.reduced (1.0f);

    g.setGradientFill (ui::chromeVertical (lever));
    g.fillRoundedRectangle (lever, corner * 0.6f);
    g.setColour (juce::Colour (0xff2b2d30));
    g.drawRoundedRectangle (lever, corner * 0.6f, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (int i = -1; i <= 1; ++i)
    {
        const float lx = lever.getCentreX() + (float) i * 4.0f;
        g.drawLine (lx, lever.getY() + 4.0f, lx, lever.getBottom() - 4.0f, 1.0f);
    }

    // ON / OFF text on the free side of the slot
    auto textArea = on ? slot.withWidth (slot.getWidth() - lw) : slot.withTrimmedLeft (lw);
    g.setColour (on ? ui::lcdGreen : ui::cream.withAlpha (0.55f));
    g.setFont (ui::makeFont (11.0f, juce::Font::bold));
    g.drawText (on ? "ON" : "OFF", textArea, juce::Justification::centred, false);
}

//==============================================================================
// Pitch slider

PitchSlider::PitchSlider (juce::RangedAudioParameter& parameter, const juce::Image& chromeTexture)
    : chromeImage (chromeTexture),
      attachment (parameter, [this] (float v)
                  {
                      index = juce::jlimit (0, 6, juce::roundToInt (v));
                      repaint();
                  }, nullptr)
{
    attachment.sendInitialUpdate();
}

juce::Rectangle<float> PitchSlider::getPlate() const
{
    return getLocalBounds().toFloat().reduced (12.0f);
}

float PitchSlider::xForIndex (int i) const
{
    const auto plate = getPlate();
    const float x0 = plate.getX() + 70.0f;
    const float step = (plate.getWidth() - 140.0f) / 6.0f;
    return x0 + step * (float) i;
}

int PitchSlider::indexFromX (float x) const
{
    const float x0 = xForIndex (0);
    const float step = xForIndex (1) - x0;
    return juce::jlimit (0, 6, juce::roundToInt ((x - x0) / step));
}

void PitchSlider::applyFromMouse (const juce::MouseEvent& e)
{
    const int i = indexFromX (e.position.x);
    if (i != index)
    {
        index = i;
        attachment.setValueAsPartOfGesture ((float) i);
        repaint();
    }
}

void PitchSlider::mouseDown (const juce::MouseEvent& e)
{
    attachment.beginGesture();
    applyFromMouse (e);
}

void PitchSlider::mouseDrag (const juce::MouseEvent& e)
{
    applyFromMouse (e);
}

void PitchSlider::mouseUp (const juce::MouseEvent&)
{
    attachment.endGesture();
}

void PitchSlider::mouseDoubleClick (const juce::MouseEvent&)
{
    index = 3; // back to unison
    attachment.setValueAsCompleteGesture (3.0f);
    repaint();
}

void PitchSlider::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const int step = wheel.deltaY > 0.0f ? 1 : (wheel.deltaY < 0.0f ? -1 : 0);
    const int next = juce::jlimit (0, 6, index + step);

    if (next != index)
    {
        index = next;
        attachment.setValueAsCompleteGesture ((float) next);
        repaint();
    }
}

void PitchSlider::paint (juce::Graphics& g)
{
    static const char* names[7]  = { "octave", "fifth", "fourth", "unison", "fourth", "fifth", "octave" };
    static const char* ratios[7] = { "2:1", "3:2", "4:3", "1:1", "4:3", "3:2", "2:1" };

    const auto plate = getPlate();
    const float cornerSize = 14.0f;

    // Chrome plate with shadow
    {
        juce::Path p;
        p.addRoundedRectangle (plate, cornerSize);
        juce::DropShadow (juce::Colours::black.withAlpha (0.65f), 12, juce::Point<int> (0, 4)).drawForPath (g, p);

        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (p);
        g.drawImage (chromeImage, plate, juce::RectanglePlacement::stretchToFit);
    }

    g.setColour (ui::ink.withAlpha (0.85f));
    g.drawRoundedRectangle (plate, cornerSize, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.65f));
    g.drawRoundedRectangle (plate.reduced (2.5f), cornerSize - 2.0f, 1.0f);

    const float trackY = plate.getY() + 84.0f;

    // Track groove
    {
        juce::Rectangle<float> track (xForIndex (0) - 22.0f, trackY - 3.0f, xForIndex (6) - xForIndex (0) + 44.0f, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.fillRoundedRectangle (track.translated (0.0f, 1.5f), 3.0f);
        g.setColour (juce::Colour (0xff2a2c2f));
        g.fillRoundedRectangle (track, 3.0f);
    }

    // The seven positions
    for (int i = 0; i < 7; ++i)
    {
        const float cx = xForIndex (i);
        const bool selected = (i == index);

        // name above the display
        g.setColour (ui::ink);
        g.setFont (ui::makeFont (11.5f, juce::Font::bold));
        g.drawText (names[i], juce::Rectangle<float> (cx - 40.0f, plate.getY() + 6.0f, 80.0f, 16.0f),
                    juce::Justification::centred, false);

        // ratio display
        juce::Rectangle<float> box (cx - 26.0f, plate.getY() + 24.0f, 52.0f, 28.0f);
        g.setColour (ui::lcdBg);
        g.fillRoundedRectangle (box, 4.0f);

        if (selected)
        {
            g.setColour (ui::lcdGreen.withAlpha (0.14f));
            g.fillRoundedRectangle (box.reduced (2.0f), 3.0f);
        }

        g.setColour (juce::Colour (0xff4a4d51));
        g.drawRoundedRectangle (box, 4.0f, 1.5f);

        g.setColour (selected ? ui::lcdGreen : ui::lcdDim);
        g.setFont (ui::makeMono (17.0f));
        g.drawText (ratios[i], box, juce::Justification::centred, false);

        // stem down to the track and a small notch
        g.setColour (ui::ink.withAlpha (0.85f));
        g.drawLine (cx, box.getBottom(), cx, trackY, 1.5f);
        g.fillEllipse (cx - 3.0f, trackY - 3.0f, 6.0f, 6.0f);
    }

    // LOWER / HIGHER bars
    {
        const float barY = plate.getY() + 112.0f;
        const float barH = 26.0f;
        const float centreX = xForIndex (3);

        juce::Rectangle<float> lower (xForIndex (0) - 30.0f, barY, (centreX - 7.0f) - (xForIndex (0) - 30.0f), barH);
        juce::Rectangle<float> higher (centreX + 7.0f, barY, (xForIndex (6) + 30.0f) - (centreX + 7.0f), barH);

        g.setColour (juce::Colour (0xff111213));
        g.fillRoundedRectangle (lower, 3.0f);
        g.setColour (juce::Colour (0xfff5f5f2));
        g.fillRoundedRectangle (higher, 3.0f);
        g.setColour (ui::ink);
        g.drawRoundedRectangle (higher, 3.0f, 1.2f);

        g.setFont (ui::makeFont (12.0f, juce::Font::bold));
        g.setColour (juce::Colours::white);
        g.drawText ("L  O  W  E  R", lower, juce::Justification::centred, false);
        g.setColour (ui::ink);
        g.drawText ("H  I  G  H  E  R", higher, juce::Justification::centred, false);
    }

    // The handle
    {
        const float hx = xForIndex (index);

        juce::Path handle;
        handle.startNewSubPath (hx - 14.0f, trackY - 30.0f);
        handle.lineTo (hx + 14.0f, trackY - 30.0f);
        handle.lineTo (hx + 14.0f, trackY - 8.0f);
        handle.lineTo (hx, trackY + 7.0f);
        handle.lineTo (hx - 14.0f, trackY - 8.0f);
        handle.closeSubPath();
        handle = handle.createPathWithRoundedCorners (3.0f);

        juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 7, juce::Point<int> (0, 3)).drawForPath (g, handle);

        g.setGradientFill (ui::chromeVertical (handle.getBounds()));
        g.fillPath (handle);
        g.setColour (juce::Colour (0xff2b2d30));
        g.strokePath (handle, juce::PathStrokeType (1.3f));

        // red indicator line (as in the original sketch)
        g.setColour (juce::Colour (0xffb3261e));
        g.fillRoundedRectangle (hx - 2.0f, trackY - 26.0f, 4.0f, 18.0f, 1.5f);
    }
}

//==============================================================================
// Detent knob (analysis window)

namespace
{
    constexpr float detentAngles[4] = { -105.0f, -35.0f, 35.0f, 105.0f };
    const char* detentLabels[4] = { "50ms", "120ms", "250ms", "400ms" };
    constexpr float knobCentreY = 68.0f;
    constexpr float knobRadius = 28.0f;
}

DetentKnob::DetentKnob (juce::RangedAudioParameter& parameter)
    : attachment (parameter, [this] (float v)
                  {
                      index = juce::jlimit (0, 3, juce::roundToInt (v));
                      repaint();
                  }, nullptr)
{
    attachment.sendInitialUpdate();
}

void DetentKnob::applyFromMouse (const juce::MouseEvent& e)
{
    const float dx = e.position.x - (float) getWidth() * 0.5f;
    const float dy = e.position.y - knobCentreY;

    if (dx * dx + dy * dy < 100.0f)
        return; // too close to the centre to tell

    const float angle = juce::radiansToDegrees (std::atan2 (dx, -dy));

    if (std::abs (angle) > 150.0f)
        return; // pointing at the bottom: ignore

    int best = 0;
    for (int i = 1; i < 4; ++i)
        if (std::abs (angle - detentAngles[i]) < std::abs (angle - detentAngles[best]))
            best = i;

    if (best != index)
    {
        index = best;
        attachment.setValueAsPartOfGesture ((float) best);
        repaint();
    }
}

void DetentKnob::mouseDown (const juce::MouseEvent& e)
{
    attachment.beginGesture();
    applyFromMouse (e);
}

void DetentKnob::mouseDrag (const juce::MouseEvent& e)
{
    applyFromMouse (e);
}

void DetentKnob::mouseUp (const juce::MouseEvent&)
{
    attachment.endGesture();
}

void DetentKnob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const int step = wheel.deltaY > 0.0f ? 1 : (wheel.deltaY < 0.0f ? -1 : 0);
    const int next = juce::jlimit (0, 3, index + step);

    if (next != index)
    {
        index = next;
        attachment.setValueAsCompleteGesture ((float) next);
        repaint();
    }
}

void DetentKnob::paint (juce::Graphics& g)
{
    const float cx = (float) getWidth() * 0.5f;
    const float cy = knobCentreY;

    for (int i = 0; i < 4; ++i)
    {
        const float a = juce::degreesToRadians (detentAngles[i]);
        const float s = std::sin (a), c = std::cos (a);
        const bool selected = (i == index);

        // small marker dot on the wood
        g.setColour (ui::cream.withAlpha (selected ? 0.95f : 0.6f));
        g.fillEllipse (cx + s * 42.0f - 2.0f, cy - c * 42.0f - 2.0f, 4.0f, 4.0f);

        // label
        juce::Rectangle<int> labelArea ((int) (cx + s * 66.0f - 26.0f), (int) (cy - c * 62.0f - 8.0f), 52, 16);
        g.setFont (ui::makeFont (12.5f, juce::Font::bold));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawText (detentLabels[i], labelArea.translated (1, 1), juce::Justification::centred, false);
        g.setColour (selected ? ui::cream : ui::cream.withAlpha (0.55f));
        g.drawText (detentLabels[i], labelArea, juce::Justification::centred, false);
    }

    ui::drawChromeKnob (g, { cx, cy }, knobRadius, juce::degreesToRadians (detentAngles[index]));
}

//==============================================================================
// Editor

HarmonioEditor::HarmonioEditor (HarmonioProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      woodImage (juce::ImageCache::getFromMemory (BinaryData::wood_jpg, BinaryData::wood_jpgSize)),
      chromeImage (juce::ImageCache::getFromMemory (BinaryData::chrome_jpg, BinaryData::chrome_jpgSize)),
      pitchSlider (*p.apvts.getParameter ("interval"), chromeImage),
      windowKnob (*p.apvts.getParameter ("window"))
{
    setLookAndFeel (&laf);
    setSize (800, 560);

    auto setupKnob = [this] (juce::Slider& s, int boxWidth, int boxHeight)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                               juce::MathConstants<float>::pi * 2.8f, true);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, boxWidth, boxHeight);
        s.setTextBoxIsEditable (true);
        s.setMouseDragSensitivity (220);
        s.setColour (juce::Slider::textBoxTextColourId, ui::lcdGreen);
        s.setColour (juce::Slider::textBoxBackgroundColourId, ui::lcdBg);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff7b7e82));
        s.setColour (juce::Slider::textBoxHighlightColourId, juce::Colour (0xff4f6b32));
        addAndMakeVisible (s);
    };

    setupKnob (formantBaseSlider, 92, 24);
    setupKnob (tonalitySlider, 92, 24);
    setupKnob (mixSlider, 100, 26);

    addAndMakeVisible (pitchSlider);
    addAndMakeVisible (windowKnob);
    addAndMakeVisible (muteButton);
    addAndMakeVisible (formantsButton);

    formantBaseAttach = std::make_unique<SliderAttachment> (proc.apvts, "formantBase", formantBaseSlider);
    tonalityAttach    = std::make_unique<SliderAttachment> (proc.apvts, "tonality", tonalitySlider);
    mixAttach         = std::make_unique<SliderAttachment> (proc.apvts, "mix", mixSlider);
    muteAttach        = std::make_unique<ButtonAttachment> (proc.apvts, "muteOriginal", muteButton);
    formantsAttach    = std::make_unique<ButtonAttachment> (proc.apvts, "formants", formantsButton);
}

HarmonioEditor::~HarmonioEditor()
{
    setLookAndFeel (nullptr);
}

void HarmonioEditor::resized()
{
    pitchSlider.setBounds (44, 112, 712, 180);

    muteButton.setBounds (64, 306, 76, 30);
    formantsButton.setBounds (64, 420, 76, 30);

    formantBaseSlider.setBounds (175, 298, 100, 124);
    tonalitySlider.setBounds (285, 298, 100, 124);
    windowKnob.setBounds (402, 296, 176, 140);
    mixSlider.setBounds (600, 292, 150, 196);
}

void HarmonioEditor::paint (juce::Graphics& g)
{
    // Wood
    g.drawImage (woodImage, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);

    // Chrome frame
    {
        auto frame = getLocalBounds().toFloat().reduced (14.0f);

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (frame.expanded (2.0f), 26.0f, 3.0f);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff3f4f6), frame.getX(), frame.getY(),
                                                 juce::Colour (0xff6a6e74), frame.getRight(), frame.getBottom(), false));
        g.drawRoundedRectangle (frame, 24.0f, 3.5f);

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (frame.reduced (4.0f), 20.0f, 1.2f);
    }

    // Title: a script font if the system has one (Windows has "Segoe Script"), else the default font
    {
        const juce::Font titleFont (juce::FontOptions ("Segoe Script", 44.0f, juce::Font::plain));
        const juce::Rectangle<int> titleArea (0, 24, getWidth(), 62);

        g.setFont (titleFont);
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.drawText ("Harmonio Sonora", titleArea.translated (2, 2), juce::Justification::centred, false);
        g.setColour (ui::cream);
        g.drawText ("Harmonio Sonora", titleArea, juce::Justification::centred, false);

        g.setFont (ui::makeFont (10.5f, juce::Font::bold));
        g.setColour (ui::cream.withAlpha (0.75f));
        g.drawText ("P E R F E C T    C O N S O N A N C E S    S H I F T E R",
                    juce::Rectangle<int> (0, 88, getWidth(), 16), juce::Justification::centred, false);
    }

    // Control labels
    ui::drawEngraved (g, "MUTE ORIGINAL SIGNAL", { 50, 340, 104, 40 }, 12.5f);
    ui::drawEngraved (g, "PRESERVE FORMANTS", { 50, 454, 104, 40 }, 12.5f);
    ui::drawEngraved (g, "FORMANT BASE", { 170, 428, 110, 22 }, 12.5f);
    ui::drawEngraved (g, "TONALITY LIMIT", { 280, 428, 110, 22 }, 12.5f);
    ui::drawEngraved (g, "ANALYSIS WINDOW", { 402, 440, 176, 22 }, 12.5f);
    ui::drawEngraved (g, "OUTPUT MIX", { 600, 490, 150, 28 }, 17.0f);
}
