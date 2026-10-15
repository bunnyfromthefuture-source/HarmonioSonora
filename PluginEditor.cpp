#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

//==============================================================================
// Layout: all positions are in pixels of the 395 x 539 background image.
namespace layout
{
    constexpr int   editorW = 395;
    constexpr int   editorH = 539;

    // Pitch slider: x of the 7 tick marks, and the handle's vertical centre / size
    constexpr float tickX[7]   = { 120.5f, 146.0f, 171.5f, 196.5f, 222.0f, 247.5f, 273.0f };
    constexpr float handleY    = 104.0f;
    constexpr float handleW    = 13.2f;
    constexpr float handleH    = 20.13f;

    // Toggle buttons (centre) and size
    const juce::Point<float> preserveCentre { 89.0f, 204.5f };
    const juce::Point<float> muteCentre     { 305.0f, 458.5f };
    constexpr float buttonSize = 34.8f;

    // Knobs (frame centre) and sizes
    const juce::Point<float> formantBaseCentre { 188.0f, 210.4f };
    const juce::Point<float> tonalityCentre    { 297.0f, 210.4f };
    const juce::Point<float> mixCentre         { 305.0f, 351.0f };
    constexpr float smallKnobSize = 85.8f;
    constexpr float bigKnobSize   = 116.6f;

    // Black value boxes in the background image
    const juce::Rectangle<int> formantBaseBox { 161, 248, 55, 15 };
    const juce::Rectangle<int> tonalityBox    { 270, 248, 55, 15 };
    const juce::Rectangle<int> mixBox         { 277, 393, 55, 15 };

    // Dialer: pivot of the disc, and size of one frame
    const juce::Point<float> dialCentre { 140.5f, 412.5f };
    constexpr float dialFrameW = 114.24f;
    constexpr float dialFrameH = 81.6f;

    // Direction in which the lever points for each setting (degrees from 12 o'clock, clockwise)
    constexpr float dialAngles[4] = { -100.0f, -59.0f, 52.0f, 100.0f };
}

namespace
{
    const juce::Colour lcdGreen { 0xff8dff72 };

    // Draws one frame of a vertical filmstrip into the destination rectangle.
    void drawFrame (juce::Graphics& g, const juce::Image& strip, int frame,
                    int frameW, int frameH, juce::Rectangle<float> dest)
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);

        auto sub = strip.getClippedImage (juce::Rectangle<int> (0, frame * frameH, frameW, frameH));
        g.drawImageTransformed (sub,
                                juce::AffineTransform::scale (dest.getWidth() / (float) frameW,
                                                              dest.getHeight() / (float) frameH)
                                    .translated (dest.getX(), dest.getY()));
    }

    juce::Rectangle<float> frameRect (juce::Point<float> editorCentre, float w, float h,
                                      juce::Point<int> componentPosition)
    {
        return juce::Rectangle<float> (w, h).withCentre (editorCentre - componentPosition.toFloat());
    }
}

//==============================================================================
// Filmstrip knob

FilmstripKnob::FilmstripKnob (const juce::Image& strip, juce::Point<float> centre, float size)
    : filmstrip (strip), frameCentre (centre), frameSize (size)
{
    numFrames = juce::jmax (1, strip.getHeight() / juce::jmax (1, strip.getWidth()));

    setSliderStyle (juce::Slider::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setMouseDragSensitivity (250);
}

void FilmstripKnob::paint (juce::Graphics& g)
{
    const double proportion = valueToProportionOfLength (getValue());
    const int frame = juce::jlimit (0, numFrames - 1, juce::roundToInt (proportion * (numFrames - 1)));

    drawFrame (g, filmstrip, frame, filmstrip.getWidth(), filmstrip.getWidth(),
               frameRect (frameCentre, frameSize, frameSize, getPosition()));
}

//==============================================================================
// Filmstrip toggle

FilmstripToggle::FilmstripToggle (const juce::Image& strip, juce::Point<float> centre, float size)
    : filmstrip (strip), frameCentre (centre), frameSize (size)
{
    setClickingTogglesState (true);
}

void FilmstripToggle::paintButton (juce::Graphics& g, bool, bool)
{
    drawFrame (g, filmstrip, getToggleState() ? 1 : 0, filmstrip.getWidth(), filmstrip.getWidth(),
               frameRect (frameCentre, frameSize, frameSize, getPosition()));
}

//==============================================================================
// Pitch slider

PitchSlider::PitchSlider (juce::RangedAudioParameter& parameter, const juce::Image& handleImage)
    : handle (handleImage),
      attachment (parameter, [this] (float v)
                  {
                      index = juce::jlimit (0, 6, juce::roundToInt (v));
                      repaint();
                  }, nullptr)
{
    attachment.sendInitialUpdate();
}

int PitchSlider::indexFromEditorX (float editorX) const
{
    int best = 0;
    for (int i = 1; i < 7; ++i)
        if (std::abs (editorX - layout::tickX[i]) < std::abs (editorX - layout::tickX[best]))
            best = i;
    return best;
}

void PitchSlider::applyFromMouse (const juce::MouseEvent& e)
{
    const int i = indexFromEditorX (e.position.x + (float) getX());

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
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);

    const auto dest = frameRect ({ layout::tickX[index], layout::handleY },
                                 layout::handleW, layout::handleH, getPosition());

    g.drawImageTransformed (handle,
                            juce::AffineTransform::scale (dest.getWidth() / (float) handle.getWidth(),
                                                          dest.getHeight() / (float) handle.getHeight())
                                .translated (dest.getX(), dest.getY()));
}

//==============================================================================
// Dialer

DialerKnob::DialerKnob (juce::RangedAudioParameter& parameter, const juce::Image& strip)
    : filmstrip (strip),
      attachment (parameter, [this] (float v)
                  {
                      index = juce::jlimit (0, 3, juce::roundToInt (v));
                      repaint();
                  }, nullptr)
{
    attachment.sendInitialUpdate();
}

void DialerKnob::applyFromMouse (const juce::MouseEvent& e)
{
    const float dx = e.position.x + (float) getX() - layout::dialCentre.x;
    const float dy = e.position.y + (float) getY() - layout::dialCentre.y;

    if (dx * dx + dy * dy < 64.0f)
        return; // too close to the pivot to tell

    const float angle = juce::radiansToDegrees (std::atan2 (dx, -dy));

    if (std::abs (angle) > 160.0f)
        return; // pointing straight down: ignore

    int best = 0;
    for (int i = 1; i < 4; ++i)
        if (std::abs (angle - layout::dialAngles[i]) < std::abs (angle - layout::dialAngles[best]))
            best = i;

    if (best != index)
    {
        index = best;
        attachment.setValueAsPartOfGesture ((float) best);
        repaint();
    }
}

void DialerKnob::mouseDown (const juce::MouseEvent& e)
{
    attachment.beginGesture();
    applyFromMouse (e);
}

void DialerKnob::mouseDrag (const juce::MouseEvent& e)
{
    applyFromMouse (e);
}

void DialerKnob::mouseUp (const juce::MouseEvent&)
{
    attachment.endGesture();
}

void DialerKnob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
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

void DialerKnob::paint (juce::Graphics& g)
{
    drawFrame (g, filmstrip, index, filmstrip.getWidth(), filmstrip.getHeight() / 4,
               frameRect (layout::dialCentre, layout::dialFrameW, layout::dialFrameH, getPosition()));
}

//==============================================================================
// Editor

HarmonioEditor::HarmonioEditor (HarmonioProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      baseImage       (juce::ImageCache::getFromMemory (BinaryData::base_png,        BinaryData::base_pngSize)),
      smallKnobsImage (juce::ImageCache::getFromMemory (BinaryData::knobs_small_png, BinaryData::knobs_small_pngSize)),
      bigKnobsImage   (juce::ImageCache::getFromMemory (BinaryData::knobs_big_png,   BinaryData::knobs_big_pngSize)),
      buttonsImage    (juce::ImageCache::getFromMemory (BinaryData::buttons_png,     BinaryData::buttons_pngSize)),
      dialerImage     (juce::ImageCache::getFromMemory (BinaryData::dialer_png,      BinaryData::dialer_pngSize)),
      sliderImage     (juce::ImageCache::getFromMemory (BinaryData::slider_png,      BinaryData::slider_pngSize)),
      pitchSlider (*p.apvts.getParameter ("interval"), sliderImage),
      windowDial (*p.apvts.getParameter ("window"), dialerImage),
      formantBaseKnob (smallKnobsImage, layout::formantBaseCentre, layout::smallKnobSize),
      tonalityKnob    (smallKnobsImage, layout::tonalityCentre,    layout::smallKnobSize),
      mixKnob         (bigKnobsImage,   layout::mixCentre,         layout::bigKnobSize),
      muteButton      (buttonsImage, layout::muteCentre,     layout::buttonSize),
      formantsButton  (buttonsImage, layout::preserveCentre, layout::buttonSize)
{
    setSize (layout::editorW, layout::editorH);

    addAndMakeVisible (pitchSlider);
    addAndMakeVisible (windowDial);
    addAndMakeVisible (formantBaseKnob);
    addAndMakeVisible (tonalityKnob);
    addAndMakeVisible (mixKnob);
    addAndMakeVisible (muteButton);
    addAndMakeVisible (formantsButton);

    // Connect to the plugin's parameters
    formantBaseAttach = std::make_unique<SliderAttachment> (proc.apvts, "formantBase", formantBaseKnob);
    tonalityAttach    = std::make_unique<SliderAttachment> (proc.apvts, "tonality", tonalityKnob);
    mixAttach         = std::make_unique<SliderAttachment> (proc.apvts, "mix", mixKnob);
    muteAttach        = std::make_unique<ButtonAttachment> (proc.apvts, "muteOriginal", muteButton);
    formantsAttach    = std::make_unique<ButtonAttachment> (proc.apvts, "formants", formantsButton);

    // The value boxes: show the knob's value, and let the user type one in
    auto setupLabel = [this] (juce::Label& label, juce::Slider& knob)
    {
        label.setJustificationType (juce::Justification::centred);
        label.setBorderSize (juce::BorderSize<int> (0, 2, 0, 2));
        label.setFont (juce::Font (juce::FontOptions ("Consolas", 11.0f, juce::Font::bold)));
        label.setEditable (true, true, false);

        label.setColour (juce::Label::textColourId, lcdGreen);
        label.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label.setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        label.setColour (juce::Label::textWhenEditingColourId, lcdGreen);
        label.setColour (juce::Label::backgroundWhenEditingColourId, juce::Colours::black);
        label.setColour (juce::Label::outlineWhenEditingColourId, lcdGreen.withAlpha (0.6f));

        knob.onValueChange = [&knob, &label]
        {
            label.setText (knob.getTextFromValue (knob.getValue()), juce::dontSendNotification);
        };

        label.onTextChange = [&knob, &label]
        {
            knob.setValue (knob.getValueFromText (label.getText()), juce::sendNotificationSync);
            knob.onValueChange(); // shows the cleaned-up value, e.g. "200 Hz"
        };

        addAndMakeVisible (label);
        knob.onValueChange();
    };

    setupLabel (formantBaseLabel, formantBaseKnob);
    setupLabel (tonalityLabel, tonalityKnob);
    setupLabel (mixLabel, mixKnob);
}

HarmonioEditor::~HarmonioEditor() = default;

void HarmonioEditor::resized()
{
    // Pitch slider: covers the tick marks and their ratio labels (so you can click on those too)
    pitchSlider.setBounds (100, 84, 195, 64);

    // Dialer: covers the dial and its four labels
    windowDial.setBounds (35, 355, 210, 100);

    formantBaseKnob.setBounds (145, 167, 87, 87);
    tonalityKnob.setBounds (254, 167, 87, 87);
    mixKnob.setBounds (246, 292, 118, 118);

    formantsButton.setBounds (71, 187, 36, 36);
    muteButton.setBounds (287, 441, 36, 36);

    formantBaseLabel.setBounds (layout::formantBaseBox);
    tonalityLabel.setBounds (layout::tonalityBox);
    mixLabel.setBounds (layout::mixBox);
}

void HarmonioEditor::paint (juce::Graphics& g)
{
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (baseImage, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
}
