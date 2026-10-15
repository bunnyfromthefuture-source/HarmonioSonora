#include "PluginProcessor.h"

HarmonioProcessor::HarmonioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout HarmonioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // The 7 positions of the pitch slider, lowest to highest.
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

void HarmonioProcessor::prepareToPlay (double, int)
{
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

    // Build-test version: audio passes through unchanged.
    // The pitch engine, mute and mix will be added in the next step.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
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
