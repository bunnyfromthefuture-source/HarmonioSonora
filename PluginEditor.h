#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

//==============================================================================
// Draws the chrome knobs and the chrome toggle switches.
class HarmonioLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HarmonioLookAndFeel() = default;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont (juce::Label&) override;
};

//==============================================================================
// The big 7-position pitch slider on the chrome plate.
class PitchSlider : public juce::Component
{
public:
    PitchSlider (juce::RangedAudioParameter& parameter, const juce::Image& chromeTexture);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::Rectangle<float> getPlate() const;
    float xForIndex (int i) const;
    int indexFromX (float x) const;
    void applyFromMouse (const juce::MouseEvent&);

    juce::Image chromeImage;
    int index = 3;
    juce::ParameterAttachment attachment;
};

//==============================================================================
// A rotary control that clicks into 4 fixed positions (the analysis window).
class DetentKnob : public juce::Component
{
public:
    explicit DetentKnob (juce::RangedAudioParameter& parameter);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void applyFromMouse (const juce::MouseEvent&);

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
    HarmonioLookAndFeel laf;

    juce::Image woodImage, chromeImage;

    PitchSlider pitchSlider;
    DetentKnob windowKnob;
    juce::Slider formantBaseSlider, tonalitySlider, mixSlider;
    juce::ToggleButton muteButton, formantsButton;

    std::unique_ptr<SliderAttachment> formantBaseAttach, tonalityAttach, mixAttach;
    std::unique_ptr<ButtonAttachment> muteAttach, formantsAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonioEditor)
};
