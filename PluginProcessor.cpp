#include "PluginProcessor.h"
#include <cmath>

namespace
{
    // Pitch ratios for the 7 slider positions (lowest to highest).
    // Down by octave 2:1, fifth 3:2, fourth 4:3 | unison | up by fourth 4:3, fifth 3:2, octave 2:1
    constexpr float pitchRatios[7] = { 1.0f / 2.0f, 2.0f / 3.0f, 3.0f / 4.0f,
                                       1.0f,
                                       4.0f / 3.0f, 3.0f / 2.0f, 2.0f / 1.0f };

    inline float readDelayed (const float* buf, int size, int writePos, float delay)
    {
        float rp = (float) writePos - delay;
        if (rp < 0.0f)
            rp += (float) size;

        const int i0 = (int) rp;
        const float frac = rp - (float) i0;
        int i1 = i0 + 1;
        if (i1 >= size)
            i1 -= size;

        return buf[i0] + frac * (buf[i1] - buf[i0]);
    }
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

    return layout;
}

void HarmonioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    windowSamples = (float) (sampleRate * (double) windowMs * 0.001);

    const int size = (int) std::ceil (windowSamples) + 8;
    delayBuffer.setSize (2, size);
    delayBuffer.clear();
    writePos = 0;
    phase = 0.0f;

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

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const int size = delayBuffer.getNumSamples();
    if (size == 0)
        return;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    const int idx = juce::jlimit (0, 6, juce::roundToInt (intervalParam->load()));
    const float ratio = pitchRatios[idx];
    const bool isUnison = (idx == 3);

    // The read position moves at 'ratio' samples per sample while writing moves at 1,
    // so the delay drifts by (1 - ratio) per sample. That drift is what changes the pitch.
    const float phaseInc = isUnison ? 0.0f : (1.0f - ratio) / windowSamples;

    mixSmooth.setTargetValue (mixParam->load() * 0.01f);
    dryGainSmooth.setTargetValue (muteParam->load() > 0.5f ? 0.0f : 1.0f);
    bypassSmooth.setTargetValue (isUnison ? 1.0f : 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const float mix    = mixSmooth.getNextValue();
        const float dryG   = dryGainSmooth.getNextValue();
        const float bypass = bypassSmooth.getNextValue();

        // Two read taps half a window apart, crossfaded with a Hann window,
        // so each tap is silent at the moment it jumps back.
        float phase2 = phase + 0.5f;
        if (phase2 >= 1.0f)
            phase2 -= 1.0f;

        const float d1 = 2.0f + phase  * windowSamples;
        const float d2 = 2.0f + phase2 * windowSamples;
        const float w1 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * phase);
        const float w2 = 1.0f - w1;

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            float* dl   = delayBuffer.getWritePointer (ch);

            const float dry = data[i];
            dl[writePos] = dry;

            const float wet = w1 * readDelayed (dl, size, writePos, d1)
                            + w2 * readDelayed (dl, size, writePos, d2);

            // Unison = true bypass. Otherwise: original (unless muted) + shifted * mix.
            data[i] = (1.0f - bypass) * (dry * dryG + wet * mix) + bypass * dry;
        }

        if (++writePos >= size)
            writePos = 0;

        phase += phaseInc;
        if (phase < 0.0f)  phase += 1.0f;
        if (phase >= 1.0f) phase -= 1.0f;
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
