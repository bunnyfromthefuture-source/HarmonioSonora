#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "signalsmith-stretch.h"

class HarmonioProcessor : public juce::AudioProcessor
{
public:
    HarmonioProcessor();
    ~HarmonioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Harmonio Sonora"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return tailSeconds; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    signalsmith::stretch::SignalsmithStretch<float> stretch;

    int channelsConfigured = 0;
    int latencySamples = 0;
    double tailSeconds = 0.0;
    bool lastFormants = false;

    juce::AudioBuffer<float> inputCopy;   // the stretcher needs separate input and output buffers
    juce::AudioBuffer<float> dryDelay;    // delays the original so it lines up with the shifted sound
    int dryPos = 0;

    juce::SmoothedValue<float> mixSmooth, dryGainSmooth, bypassSmooth;

    std::atomic<float>* intervalParam = nullptr;
    std::atomic<float>* muteParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* formantsParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonioProcessor)
};
