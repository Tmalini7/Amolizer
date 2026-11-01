#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

class AmolizerProcessor : public juce::AudioProcessor
{
public:
    AmolizerProcessor();
    ~AmolizerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    void updateDsp (float amount, int mode);

    using Filter       = juce::dsp::IIR::Filter<float>;
    using Coefs        = juce::dsp::IIR::Coefficients<float>;
    using StereoFilter = juce::dsp::ProcessorDuplicator<Filter, Coefs>;

    // 0: low cut, 1: mud cut, 2: presence, 3: air shelf
    juce::dsp::ProcessorChain<StereoFilter, StereoFilter, StereoFilter, StereoFilter> eq;

    // Split-band de-esser
    juce::dsp::LinkwitzRileyFilter<float> deessSplit;
    std::array<float, 2> deessEnv { 0.0f, 0.0f };
    float deessAttack = 0.0f, deessRelease = 0.0f;
    float deessThresh = 1.0f, deessRatio = 4.0f;

    juce::dsp::Compressor<float> comp;
    juce::dsp::Gain<float> makeup, output;
    juce::dsp::Limiter<float> limiter;

    float drive = 1.0f, satAmount = 0.0f;

    std::atomic<float>* amountParam = nullptr;
    std::atomic<float>* modeParam   = nullptr;
    std::atomic<float>* outputParam = nullptr;

    double sr = 44100.0;
    float lastAmount = -1.0f;
    int lastMode = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmolizerProcessor)
};
