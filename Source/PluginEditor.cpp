#include "PluginEditor.h"
#include "BinaryData.h"

// Works with both JUCE 7 and JUCE 8
static juce::Font makeFont (float size, bool bold)
{
   #if JUCE_MAJOR_VERSION >= 8
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
   #else
    return juce::Font (size, bold ? juce::Font::bold : juce::Font::plain);
   #endif
}

namespace Palette
{
    const juce::Colour track   { 0xff2a2f3a };
    const juce::Colour knob    { 0xff1b1f27 };
    const juce::Colour rim     { 0xff3a4150 };
    const juce::Colour accentA { 0xffff7a59 };
    const juce::Colour accentB { 0xffffc857 };
    const juce::Colour text    { 0xffe8eaf0 };
    const juce::Colour dim     { 0xff8a91a0 };
    const juce::Colour panel   { 0xcc11141a };
}

namespace Layout
{
    // Editor size (artwork is stored at 2x for sharp HiDPI rendering)
    constexpr int width = 480, height = 606;

    // Where the tongue knob lives in the editor
    const juce::Rectangle<int> knobArea { 150, 190, 280, 295 };

    // Root of the tongue (rotation pivot) in editor coordinates
    constexpr float pivotX = 290.2f, pivotY = 337.3f;

    // Top-left of the tongue cut-out in editor coordinates
    constexpr float tongueX = 240.5f, tongueY = 331.0f;

    // Swing range either side of straight down
    constexpr float swingDeg = 55.0f;
    constexpr float arcRadius = 128.0f;
}

//==============================================================================
GlowLookAndFeel::GlowLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, Palette::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TextButton::buttonColourId, Palette::panel);
    setColour (juce::TextButton::buttonOnColourId, Palette::accentA);
    setColour (juce::TextButton::textColourOffId, Palette::text);
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff11141a));
    setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, Palette::dim);
}

void GlowLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                        float pos, float start, float end, juce::Slider&)
{
    using namespace juce;

    auto bounds = Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float r = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const float lineW = jmax (3.0f, r * 0.12f);
    const float arcR = r - lineW * 0.5f;
    const PathStrokeType stroke (lineW, PathStrokeType::curved, PathStrokeType::rounded);

    Path bg;
    bg.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, end, true);
    g.setColour (Palette::track);
    g.strokePath (bg, stroke);

    if (pos > 0.0f)
    {
        Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, angle, true);
        g.setGradientFill (ColourGradient (Palette::accentA, c.x - r, c.y,
                                           Palette::accentB, c.x + r, c.y, false));
        g.strokePath (val, stroke);
    }

    const float knobR = arcR - lineW * 1.5f;
    g.setColour (Palette::knob);
    g.fillEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f);
    g.setColour (Palette::rim);
    g.drawEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f, 1.5f);

    Path pointer;
    const float pw = lineW * 0.6f;
    pointer.addRoundedRectangle (-pw * 0.5f, -knobR + lineW * 0.6f, pw, knobR * 0.45f, pw * 0.5f);
    g.setColour (Palette::text);
    g.fillPath (pointer, AffineTransform::rotation (angle).translated (c.x, c.y));
}

//==============================================================================
TongueKnob::TongueKnob()
{
    tongue = juce::ImageCache::getFromMemory (BinaryData::tongue_png, BinaryData::tongue_pngSize);
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setMouseDragSensitivity (220);
    setMouseCursor (juce::MouseCursor::UpDownLeftRightResizeCursor);
    setTooltip ("Drag to Amolize");
}

void TongueKnob::paint (juce::Graphics& g)
{
    using namespace juce;
    using namespace Layout;

    // Draw in editor coordinates
    g.addTransform (AffineTransform::translation ((float) -getX(), (float) -getY()));

    const float pos = (float) valueToProportionOfLength (getValue());
    const float swing = degreesToRadians (swingDeg);
    const float rot = (pos - 0.5f) * 2.0f * swing;      // + = clockwise on screen
    const float startA = MathConstants<float>::pi - swing;
    const float endA   = MathConstants<float>::pi + swing;
    const float tipA   = MathConstants<float>::pi + rot;

    // Glow arc around the tongue
    const PathStrokeType stroke (7.0f, PathStrokeType::curved, PathStrokeType::rounded);
    Path track;
    track.addCentredArc (pivotX, pivotY, arcRadius, arcRadius, 0.0f, startA, endA, true);
    g.setColour (Colours::black.withAlpha (0.45f));
    g.strokePath (track, stroke);

    if (pos > 0.001f)
    {
        Path val;
        val.addCentredArc (pivotX, pivotY, arcRadius, arcRadius, 0.0f, startA, tipA, true);
        g.setGradientFill (ColourGradient (Palette::accentB, pivotX + arcRadius, pivotY,
                                           Palette::accentA, pivotX - arcRadius, pivotY, false));
        g.strokePath (val, stroke);
    }

    // The tongue itself (image is 2x, so scale by 0.5)
    const auto t = AffineTransform::scale (0.5f)
                     .translated (tongueX, tongueY)
                     .rotated (rot, pivotX, pivotY);
    g.drawImageTransformed (tongue, t);
}

//==============================================================================
AmolizerEditor::AmolizerEditor (AmolizerProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    background = juce::ImageCache::getFromMemory (BinaryData::background_jpg, BinaryData::background_jpgSize);

    addAndMakeVisible (amount);
    amountAtt = std::make_unique<SliderAttachment> (proc.apvts, "amount", amount);
    amount.onValueChange = [this] { repaint(); };

    output.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    output.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    output.setTextValueSuffix (" dB");
    output.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (output);
    outputAtt = std::make_unique<SliderAttachment> (proc.apvts, "output", output);

    outputLabel.setText ("OUTPUT", juce::dontSendNotification);
    outputLabel.setJustificationType (juce::Justification::centred);
    outputLabel.attachToComponent (&output, false);

    const char* names[] = { "Smooth", "Bright", "Warm", "Punchy" };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = modeButtons[(size_t) i];
        b.setButtonText (names[i]);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (1001);
        b.onClick = [this, i]
        {
            if (modeButtons[(size_t) i].getToggleState() && modeAtt != nullptr)
                modeAtt->setValueAsCompleteGesture ((float) i);
        };
        addAndMakeVisible (b);
    }

    modeAtt = std::make_unique<juce::ParameterAttachment> (
        *proc.apvts.getParameter ("mode"),
        [this] (float v)
        {
            const int idx = juce::jlimit (0, 3, (int) std::round (v));
            modeButtons[(size_t) idx].setToggleState (true, juce::dontSendNotification);
        },
        nullptr);
    modeAtt->sendInitialUpdate();

    setSize (Layout::width, Layout::height);
}

AmolizerEditor::~AmolizerEditor()
{
    setLookAndFeel (nullptr);
}

void AmolizerEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.drawImage (background, getLocalBounds().toFloat());

    // Title with a drop shadow so it reads on the green wall
    auto title = juce::Rectangle<int> (18, 14, 300, 44);
    g.setFont (makeFont (36.0f, true));
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("AMOLIZER", title.translated (2, 2), juce::Justification::centredLeft);
    g.setColour (juce::Colours::white);
    g.drawText ("AMOLIZER", title, juce::Justification::centredLeft);

    // Amount readout over the shirt
    const int pct = juce::roundToInt (amount.getValue());
    auto readout = juce::Rectangle<int> (22, 482, 200, 56);
    g.setColour (Palette::dim);
    g.setFont (makeFont (13.0f, true));
    g.drawText ("AMOLIZATION", readout.removeFromTop (18), juce::Justification::centredLeft);
    g.setColour (Palette::accentB);
    g.setFont (makeFont (32.0f, true));
    g.drawText (juce::String (pct) + "%", readout, juce::Justification::centredLeft);
}

void AmolizerEditor::resized()
{
    amount.setBounds (Layout::knobArea);
    output.setBounds (380, 478, 86, 80);

    auto buttons = getLocalBounds().reduced (14).removeFromBottom (30);
    const int bw = buttons.getWidth() / 4;
    for (auto& b : modeButtons)
        b.setBounds (buttons.removeFromLeft (bw).reduced (3, 0));
}
