#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class GlowLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GlowLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;
};

// The main knob: the tongue swings around its root as you drag.
class TongueKnob : public juce::Slider
{
public:
    TongueKnob();
    void paint (juce::Graphics&) override;

private:
    juce::Image tongue;
};

class AmolizerEditor : public juce::AudioProcessorEditor
{
public:
    explicit AmolizerEditor (AmolizerProcessor&);
    ~AmolizerEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    AmolizerProcessor& proc;
    GlowLookAndFeel lnf;
    juce::Image background;

    TongueKnob amount;
    juce::Slider output;
    juce::Label outputLabel;
    std::unique_ptr<SliderAttachment> amountAtt, outputAtt;

    std::array<juce::TextButton, 4> modeButtons;
    std::unique_ptr<juce::ParameterAttachment> modeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmolizerEditor)
};
