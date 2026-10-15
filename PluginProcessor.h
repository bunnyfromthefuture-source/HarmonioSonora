#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

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
    double getTailLengthSeconds() const override { return 0.0; }

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

    // Length of the crossfading grain window in milliseconds.
    // Longer = smoother on low notes but more "echo"; shorter = tighter but more warble.
    static constexpr float windowMs = 40.0f;

    juce::AudioBuffer<float> delayBuffer;
    int writePos = 0;
    float phase = 0.0f;
    float windowSamples = 1764.0f;
    double currentSampleRate = 44100.0;

    juce::SmoothedValue<float> mixSmooth, dryGainSmooth, bypassSmooth;

    std::atomic<float>* intervalParam = nullptr;
    std::atomic<float>* muteParam = nullptr;
    std::atomic<float>* mixParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonioProcessor)
};
