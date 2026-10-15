#include "PluginProcessor.h"
#include <cmath>

namespace
{
    // Exact pitch ratios for the 7 slider positions (lowest to highest).
    // Down: octave 2:1, fifth 3:2, fourth 4:3 | unison | Up: fourth 4:3, fifth 3:2, octave 2:1
    constexpr float pitchRatios[7] = { 1.0f / 2.0f, 2.0f / 3.0f, 3.0f / 4.0f,
                                       1.0f,
                                       4.0f / 3.0f, 3.0f / 2.0f, 2.0f / 1.0f };
}

HarmonioProcessor::HarmonioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    intervalParam = apvts.getRawParameterValue ("interval");
    muteParam     = apvts.getRawParameterValue ("muteOriginal");
    mixParam      = apvts.getRawParameterValue ("mix");
    formantModeParam = apvts.getRawParameterValue ("formantMode");
    tonalityParam    = apvts.getRawParameterValue ("tonality");
    formantBaseParam = apvts.getRawParameterValue ("formantBase");
}

juce::AudioProcessorValueTreeState::ParameterLayout HarmonioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::StringArray intervals { "Octave down (2:1)",
                                  "Fifth down (3:2)",
                                  "Fourth down (4:3)",
                                  "Unison (1:1)",
                                  "Fourth up (4:3)",
                                  "Fifth up (3:2)",
                                  "Octave up (2:1)" };

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "interval", 1 }, "Pitch", intervals, 3));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "muteOriginal", 1 }, "Original Signal Mute", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    // Keeps the "character" of voices/instruments (their body resonance) in place
    // instead of shifting it along with the pitch.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "formantMode", 1 }, "Preserve Formants",
        juce::StringArray { "Off", "Up-shifts only", "Always" }, 1));

    // Rough pitch of the source (e.g. ~200 Hz for a voice, ~100 Hz for a bass).
    // 0 = Auto (the plugin guesses, which can be unreliable).
    // Tonality limit: above this frequency the shift is applied as a plain offset
    // instead of a multiplication, which keeps more of the original brightness
    // (mainly useful when pitching down). 0 = Off.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tonality", 1 }, "Tonality Limit",
        juce::NormalisableRange<float> (0.0f, 16000.0f, 10.0f, 0.4f), 0.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int)
            {
                return v < 1.0f ? juce::String ("Off")
                                : juce::String (juce::roundToInt (v)) + " Hz";
            })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "formantBase", 1 }, "Formant Base",
        juce::NormalisableRange<float> (0.0f, 1000.0f, 1.0f, 0.4f), 0.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int)
            {
                return v < 1.0f ? juce::String ("Auto")
                                : juce::String (juce::roundToInt (v)) + " Hz";
            })));

    return layout;
}

void HarmonioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    channelsConfigured = juce::jlimit (1, 2, getTotalNumInputChannels());

    // Quality-first setup (the "default" preset, not the cheaper one).
    stretch.presetDefault (channelsConfigured, (float) sampleRate);
    stretch.reset();

    lastFormants = false;
    stretch.setFormantFactor (1.0f, false);

    currentSampleRate = sampleRate;
    lastFormantBase = formantBaseParam->load();
    stretch.setFormantBase (lastFormantBase > 0.5f ? lastFormantBase / (float) sampleRate : 0.0f);

    // Tell the host how late the shifted sound arrives, so Reaper can compensate.
    latencySamples = stretch.inputLatency() + stretch.outputLatency();
    setLatencySamples (latencySamples);
    tailSeconds = (double) latencySamples / sampleRate;

    inputCopy.setSize (channelsConfigured, juce::jmax (samplesPerBlock, 1));
    inputCopy.clear();

    dryDelay.setSize (channelsConfigured, juce::jmax (latencySamples, 1));
    dryDelay.clear();
    dryPos = 0;

    mixSmooth.reset (sampleRate, 0.03);
    dryGainSmooth.reset (sampleRate, 0.03);
    bypassSmooth.reset (sampleRate, 0.03);

    mixSmooth.setCurrentAndTargetValue (mixParam->load() * 0.01f);
    dryGainSmooth.setCurrentAndTargetValue (muteParam->load() > 0.5f ? 0.0f : 1.0f);
    bypassSmooth.setCurrentAndTargetValue (juce::roundToInt (intervalParam->load()) == 3 ? 1.0f : 0.0f);
}

bool HarmonioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return in == out;
}

void HarmonioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    if (channelsConfigured == 0 || buffer.getNumChannels() != channelsConfigured || numSamples == 0)
        return;

    if (inputCopy.getNumSamples() < numSamples)
        inputCopy.setSize (channelsConfigured, numSamples, false, false, true);

    for (int c = 0; c < channelsConfigured; ++c)
        inputCopy.copyFrom (c, 0, buffer, c, 0, numSamples);

    // Parameters
    const int idx = juce::jlimit (0, 6, juce::roundToInt (intervalParam->load()));
    const bool isUnison = (idx == 3);
    const int formantMode = juce::roundToInt (formantModeParam->load()); // 0 Off, 1 Up-shifts only, 2 Always
    const bool formants = (formantMode == 2) || (formantMode == 1 && idx > 3);

    const float tonalityHz = tonalityParam->load();
    stretch.setTransposeFactor (pitchRatios[idx],
                                tonalityHz > 1.0f ? tonalityHz / (float) currentSampleRate : 0.0f);

    if (formants != lastFormants)
    {
        stretch.setFormantFactor (1.0f, formants);
        lastFormants = formants;
    }

    const float formantBase = formantBaseParam->load();
    if (formantBase != lastFormantBase)
    {
        stretch.setFormantBase (formantBase > 0.5f ? formantBase / (float) currentSampleRate : 0.0f);
        lastFormantBase = formantBase;
    }

    mixSmooth.setTargetValue (mixParam->load() * 0.01f);
    dryGainSmooth.setTargetValue (muteParam->load() > 0.5f ? 0.0f : 1.0f);
    bypassSmooth.setTargetValue (isUnison ? 1.0f : 0.0f);

    // The shifter always runs (even at unison) so switching never causes a jump.
    const float* const* in  = inputCopy.getArrayOfReadPointers();
    float* const*       out = buffer.getArrayOfWritePointers();

    stretch.process (in, numSamples, out, numSamples);

    // Blend: original (delayed to line up with the shifted sound) + shifted sound.
    for (int i = 0; i < numSamples; ++i)
    {
        const float mix    = mixSmooth.getNextValue();
        const float dryG   = dryGainSmooth.getNextValue();
        const float bypass = bypassSmooth.getNextValue();

        for (int c = 0; c < channelsConfigured; ++c)
        {
            const float dryNow = in[c][i];
            float dry = dryNow;

            if (latencySamples > 0)
            {
                float* d = dryDelay.getWritePointer (c);
                dry = d[dryPos];
                d[dryPos] = dryNow;
            }

            const float wet = out[c][i];

            // Unison = bypass (original only). Otherwise: original (unless muted) + shifted * mix.
            out[c][i] = (1.0f - bypass) * (dry * dryG + wet * mix) + bypass * dry;
        }

        if (latencySamples > 0 && ++dryPos >= latencySamples)
            dryPos = 0;
    }
}

juce::AudioProcessorEditor* HarmonioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void HarmonioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void HarmonioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HarmonioProcessor();
}
