#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    // Each mode is a "character". All values are what you get at Amount = 100%;
    // everything scales smoothly down to neutral at 0%.
    struct ModeProfile
    {
        float mudDb;          // cut around 300 Hz (boxiness)
        float presenceDb;     // presence boost
        float presenceHz;
        float airDb;          // high shelf for sheen
        float compThreshDb;   // compressor threshold
        float compRatio;
        float drive;          // saturation drive
        float satMix;         // how much saturation is blended in
        float deessDb;        // de-esser threshold (lower = more de-essing)
    };

    constexpr ModeProfile kModes[] =
    {
        // Smooth: polished, even, gentle sheen
        { -3.0f, 2.0f, 3000.0f, 2.5f, -20.0f, 3.0f, 1.8f, 0.35f, -30.0f },
        // Bright: forward and airy, extra de-essing to tame the top end
        { -4.0f, 4.0f, 4200.0f, 5.0f, -22.0f, 3.5f, 1.6f, 0.30f, -34.0f },
        // Warm: thicker, more saturation, softer top
        { -1.5f, 1.5f, 2500.0f, 1.0f, -18.0f, 3.0f, 3.0f, 0.60f, -28.0f },
        // Punchy: heavy compression, strong mids, sits on top of a busy mix
        { -5.0f, 5.0f, 2000.0f, 2.0f, -28.0f, 6.0f, 2.5f, 0.45f, -30.0f },
    };
}

AmolizerProcessor::AmolizerProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    amountParam = apvts.getRawParameterValue ("amount");
    modeParam   = apvts.getRawParameterValue ("mode");
    outputParam = apvts.getRawParameterValue ("output");
}

juce::AudioProcessorValueTreeState::ParameterLayout AmolizerProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "amount", 1 }, "Amount",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 40.0f,
        AudioParameterFloatAttributes().withLabel ("%")));

    p.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { "mode", 1 }, "Mode",
        StringArray { "Smooth", "Bright", "Warm", "Punchy" }, 0));

    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "output", 1 }, "Output",
        NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    return { p.begin(), p.end() };
}

bool AmolizerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void AmolizerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate,
                                  (juce::uint32) samplesPerBlock,
                                  (juce::uint32) getTotalNumOutputChannels() };

    eq.prepare (spec);
    eq.reset();

    deessSplit.prepare (spec);
    deessSplit.setCutoffFrequency (juce::jmin (5500.0f, (float) sampleRate * 0.4f));
    deessEnv.fill (0.0f);
    deessAttack  = std::exp (-1.0f / (0.001f * (float) sampleRate));  // 1 ms
    deessRelease = std::exp (-1.0f / (0.060f * (float) sampleRate));  // 60 ms

    comp.prepare (spec);
    comp.setAttack (8.0f);
    comp.setRelease (120.0f);

    makeup.prepare (spec);
    makeup.setRampDurationSeconds (0.05);
    output.prepare (spec);
    output.setRampDurationSeconds (0.05);

    limiter.prepare (spec);
    limiter.setThreshold (-1.0f);
    limiter.setRelease (80.0f);

    lastAmount = -1.0f;
    lastMode = -1;
}

void AmolizerProcessor::updateDsp (float a, int mode)
{
    const auto& p = kModes[juce::jlimit (0, 3, mode)];
    auto db = [] (float d) { return juce::Decibels::decibelsToGain (d); };
    const float nyqSafe = (float) sr * 0.45f;

    *eq.get<0>().state = *Coefs::makeHighPass  (sr, 20.0f + 70.0f * a, 0.707f);
    *eq.get<1>().state = *Coefs::makePeakFilter (sr, 300.0f, 1.0f, db (p.mudDb * a));
    *eq.get<2>().state = *Coefs::makePeakFilter (sr, juce::jmin (p.presenceHz, nyqSafe), 0.8f, db (p.presenceDb * a));
    *eq.get<3>().state = *Coefs::makeHighShelf  (sr, juce::jmin (11000.0f, nyqSafe), 0.7f, db (p.airDb * a));

    deessThresh = db (p.deessDb * a);   // 0 dB at Amount 0 => effectively off

    const float thresh = p.compThreshDb * a;
    const float ratio  = 1.0f + (p.compRatio - 1.0f) * a;
    comp.setThreshold (thresh);
    comp.setRatio (ratio);

    // Rough auto makeup so turning the knob doesn't just make it quieter
    makeup.setGainDecibels (-thresh * (1.0f - 1.0f / ratio) * 0.4f);

    drive = 1.0f + (p.drive - 1.0f) * a;
    satAmount = p.satMix * a;
}

void AmolizerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const float a = amountParam->load() / 100.0f;
    const int mode = (int) modeParam->load();
    if (std::abs (a - lastAmount) > 1.0e-6f || mode != lastMode)
    {
        updateDsp (a, mode);
        lastAmount = a;
        lastMode = mode;
    }
    output.setGainDecibels (outputParam->load());

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    // 1) Tone shaping
    eq.process (ctx);

    // 2) De-esser + 3) saturation, per sample
    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        float& env = deessEnv[(size_t) ch];

        for (int n = 0; n < numSamples; ++n)
        {
            float lo = 0.0f, hi = 0.0f;
            deessSplit.processSample (ch, d[n], lo, hi);

            const float level = std::abs (hi);
            const float coef = level > env ? deessAttack : deessRelease;
            env = coef * env + (1.0f - coef) * level;

            float g = 1.0f;
            if (env > deessThresh)
                g = std::pow (deessThresh / env, 1.0f - 1.0f / deessRatio);

            const float x = lo + hi * g;
            const float sat = std::tanh (drive * x) / drive;
            d[n] = x + satAmount * (sat - x);
        }
    }

    // 4) Glue compression, 5) gain staging, 6) safety limiter
    comp.process (ctx);
    makeup.process (ctx);
    output.process (ctx);
    limiter.process (ctx);
}

void AmolizerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AmolizerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* AmolizerProcessor::createEditor()
{
    return new AmolizerEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AmolizerProcessor();
}
