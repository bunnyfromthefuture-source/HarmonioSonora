#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

//==============================================================================
// A rotary knob drawn from a "filmstrip" image (128 frames stacked vertically).
class FilmstripKnob : public juce::Slider
{
public:
    // centre and size are given in the editor's coordinates (pixels)
    FilmstripKnob (const juce::Image& strip, juce::Point<float> centre, float size);
    void paint (juce::Graphics&) override;

private:
    juce::Image filmstrip;
    int numFrames = 1;
    juce::Point<float> frameCentre;
    float frameSize = 0.0f;
};

//==============================================================================
// An on/off button drawn from a 2-frame image (off on top, lit on the bottom).
class FilmstripToggle : public juce::ToggleButton
{
public:
    FilmstripToggle (const juce::Image& strip, juce::Point<float> centre, float size);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    juce::Image filmstrip;
    juce::Point<float> frameCentre;
    float frameSize = 0.0f;
};

//==============================================================================
// The pitch slider: a small handle that snaps onto the 7 tick marks.
class PitchSlider : public juce::Component
{
public:
    PitchSlider (juce::RangedAudioParameter& parameter, const juce::Image& handleImage);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    int indexFromEditorX (float editorX) const;
    void applyFromMouse (const juce::MouseEvent&);

    juce::Image handle;
    int index = 3;
    juce::ParameterAttachment attachment;
};

//==============================================================================
// The analysis-window dial: clicks into 4 fixed positions.
class DialerKnob : public juce::Component
{
public:
    DialerKnob (juce::RangedAudioParameter& parameter, const juce::Image& strip);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void applyFromMouse (const juce::MouseEvent&);

    juce::Image filmstrip;
    int index = 1;
    juce::ParameterAttachment attachment;
};

//==============================================================================
class HarmonioEditor : public juce::AudioProcessorEditor
{
public:
    explicit HarmonioEditor (HarmonioProcessor&);
    ~HarmonioEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    HarmonioProcessor& proc;

    juce::Image baseImage, smallKnobsImage, bigKnobsImage, buttonsImage, dialerImage, sliderImage;

    PitchSlider pitchSlider;
    DialerKnob windowDial;
    FilmstripKnob formantBaseKnob, tonalityKnob, mixKnob;
    FilmstripToggle muteButton, formantsButton;
    juce::Label formantBaseLabel, tonalityLabel, mixLabel;

    std::unique_ptr<SliderAttachment> formantBaseAttach, tonalityAttach, mixAttach;
    std::unique_ptr<ButtonAttachment> muteAttach, formantsAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonioEditor)
};
