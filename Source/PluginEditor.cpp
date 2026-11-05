#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
LoFibyPAPIELGUAPOAudioProcessorEditor::LoFibyPAPIELGUAPOAudioProcessorEditor (LoFibyPAPIELGUAPOAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&customLookAndFeel);

    // LED de bypass (solo punto, sin texto)
    bypassLed.setClickingTogglesState (true);
    addAndMakeVisible (bypassLed);
    bypassAttachment = std::make_unique<juce::ParameterAttachment> (
        *audioProcessor.apvts.getParameter ("bypass"),
        [this] (float v) { bypassLed.setToggleState (v < 0.5f, juce::dontSendNotification); });
    bypassLed.onClick = [this]
    {
        bypassAttachment->setValueAsCompleteGesture (bypassLed.getToggleState() ? 0.0f : 1.0f);
    };
    bypassAttachment->sendInitialUpdate();

    // Quant Mode: dos botones (Linear / Adaptive)
    linearButton.setRadioGroupId (1);
    adaptiveButton.setRadioGroupId (1);
    linearButton.setClickingTogglesState (true);
    adaptiveButton.setClickingTogglesState (true);
    addAndMakeVisible (linearButton);
    addAndMakeVisible (adaptiveButton);
    quantAttachment = std::make_unique<juce::ParameterAttachment> (
        *audioProcessor.apvts.getParameter ("quantMode"),
        [this] (float v)
        {
            const bool adaptive = v > 0.5f;
            linearButton.setToggleState (! adaptive, juce::dontSendNotification);
            adaptiveButton.setToggleState (adaptive, juce::dontSendNotification);
        });
    linearButton.onClick   = [this] { quantAttachment->setValueAsCompleteGesture (0.0f); };
    adaptiveButton.onClick = [this] { quantAttachment->setValueAsCompleteGesture (1.0f); };
    quantAttachment->sendInitialUpdate();

    auto setupLabel = [this] (juce::Label& label, const juce::String& text, juce::Justification just)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (just);
        label.setColour (juce::Label::textColourId, juce::Colour (0xffe6e9ec));
        addAndMakeVisible (label);
    };

    auto setupHSlider = [this] (juce::Slider& s, const juce::String& suffix)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 90, 20);
        s.setTextValueSuffix (suffix);
        addAndMakeVisible (s);
    };

    auto setupVSlider = [this] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 52, 18);
        s.setTextValueSuffix (" dB");
        s.setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (s);
    };

    auto setupKnob = [this] (juce::Slider& s, const juce::String& suffix)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
        s.setTextValueSuffix (suffix);
        addAndMakeVisible (s);
    };

    const auto left = juce::Justification::centredLeft;
    const auto centre = juce::Justification::centred;

    setupLabel (sampleRateRedLabel, "Sample Rate", left);
    setupHSlider (sampleRateRedSlider, "");
    sampleRateRedAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "sampleRateRed", sampleRateRedSlider);
    sampleRateRedSlider.textFromValueFunction = [] (double v) { return v > 48000.5 ? juce::String ("Off") : juce::String (juce::roundToInt (v)) + " Hz"; };
    sampleRateRedSlider.valueFromTextFunction = [] (const juce::String& t) { return t.containsIgnoreCase ("off") ? 48001.0 : t.getDoubleValue(); };
    sampleRateRedSlider.updateText();

    setupLabel (sampleSizeLabel, "Sample Size", left);
    setupHSlider (sampleSizeSlider, "");
    bitsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "bitDepth", sampleSizeSlider);
    sampleSizeSlider.setNumDecimalPlacesToDisplay (0);
    sampleSizeSlider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v)) + " Bits"; };
    sampleSizeSlider.valueFromTextFunction = [] (const juce::String& s) { return s.getDoubleValue(); };
    sampleSizeSlider.updateText();

    setupLabel (antiAliasLabel, "Anti-Alias", left);
    setupHSlider (antiAliasSlider, " %");
    antiAliasAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "antiAlias", antiAliasSlider);

    setupLabel (distortionLabel, "Distortion", centre);
    setupKnob (distortionKnob, " dB");
    distortionAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "distortion", distortionKnob);

    setupLabel (noiseLabel, "Tape", centre);
    setupKnob (noiseKnob, " %");
    noiseAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "tape", noiseKnob);

    setupLabel (saturationLabel, "Saturation", centre);
    setupKnob (saturationKnob, " dB");
    saturationAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "saturation", saturationKnob);

    setupLabel (evenLabel, "Even (2nd)", left);
    setupHSlider (evenKnob, " %");
    evenAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "even", evenKnob);

    setupLabel (oddLabel, "Odd (3rd)", left);
    setupHSlider (oddKnob, " %");
    oddAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "odd", oddKnob);

    setupLabel (trimLabel, "Trim", left);
    setupVSlider (trimSlider);
    trimAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "trim", trimSlider);
    setupVSlider (inTrimSlider);
    inTrimAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, "inTrim", inTrimSlider);

    addAndMakeVisible (inMeter);
    addAndMakeVisible (outMeter);

    const char* kNames[3] = { "K-12", "K-14", "K-20" };
    for (int i = 0; i < 3; ++i)
    {
        kButtons[i].setButtonText (kNames[i]);
        kButtons[i].setRadioGroupId (2);
        kButtons[i].setClickingTogglesState (true);
        kButtons[i].onClick = [this, i] { kAttachment->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (kButtons[i]);
    }
    kAttachment = std::make_unique<juce::ParameterAttachment> (
        *audioProcessor.apvts.getParameter ("kScale"),
        [this] (float v)
        {
            const int idx = juce::jlimit (0, 2, juce::roundToInt (v));
            kRef = idx == 0 ? 12 : (idx == 1 ? 14 : 20);
            for (int i = 0; i < 3; ++i)
                kButtons[i].setToggleState (i == idx, juce::dontSendNotification);
        });
    kAttachment->sendInitialUpdate();

    setResizable (true, true);
    setResizeLimits (280, 221, 1400, 1103);
    getConstrainer()->setFixedAspectRatio (660.0 / 520.0);
    setSize (660, 520);
    startTimerHz (30);
}

LoFibyPAPIELGUAPOAudioProcessorEditor::~LoFibyPAPIELGUAPOAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void LoFibyPAPIELGUAPOAudioProcessorEditor::timerCallback()
{
	// RMS con +3 dB (0 K = -kRef dBFS RMS para una senoidal) y picos en dBFS relativos a 0 K
	auto rmsK  = [this] (float ms) { return 10.0f * std::log10 (ms + 1.0e-12f) + 3.0f + (float) kRef; };
	auto peakK = [this] (float p)  { return 20.0f * std::log10 (p + 1.0e-6f) + (float) kRef; };

	inMeter.setLevels (rmsK (audioProcessor.meterIn[0].load()), rmsK (audioProcessor.meterIn[1].load()),
					   peakK (audioProcessor.peakIn[0].exchange (0.0f)), peakK (audioProcessor.peakIn[1].exchange (0.0f)));
	outMeter.setLevels (rmsK (audioProcessor.meterOut[0].load()), rmsK (audioProcessor.meterOut[1].load()),
						peakK (audioProcessor.peakOut[0].exchange (0.0f)), peakK (audioProcessor.peakOut[1].exchange (0.0f)));
}
//==============================================================================
void LoFibyPAPIELGUAPOAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1b1e20));

    const float s = (float) getWidth() / 660.0f;
    auto S = [s] (int v) { return juce::roundToInt ((float) v * s); };
    const int x0 = S (112), cw = getWidth() - x0 * 2;

    g.setColour (juce::Colour (0xff141618));
    g.fillRect (0, 0, getWidth(), S (44));
    g.setColour (juce::Colour (0xffe5e9f0));
    g.setFont (juce::FontOptions (17.0f * s, juce::Font::bold));
    g.drawText ("DADD", S (16), 0, S (100), S (44), juce::Justification::centredLeft, true);

    auto panel = [&] (juce::Rectangle<int> b, const juce::String& title)
    {
        g.setColour (juce::Colour (0xff222629));
        g.fillRoundedRectangle (b.toFloat(), 5.0f * s);
        g.setColour (juce::Colour (0xff2e3438));
        g.drawRoundedRectangle (b.toFloat(), 5.0f * s, 1.0f);
        g.setColour (juce::Colour (0xffe6e9ec));
        g.setFont (juce::FontOptions (14.0f * s, juce::Font::bold));
        g.drawText (title, b.getX() + S (12), b.getY() + S (6), b.getWidth() - S (24), S (22), juce::Justification::centredLeft, true);
    };

    const int m = S (14), w = cw - m * 2;
    panel ({ x0 + m, S (92),  w, S (136) }, "Conversion");
    panel ({ x0 + m, S (238), w, S (104) }, "Harmonics");
    panel ({ x0 + m, S (352), w, S (150) }, "Grunge");
}

void LoFibyPAPIELGUAPOAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / 660.0f;
    auto S = [s] (int v) { return juce::roundToInt ((float) v * s); };
    const int x0 = S (112), cw = getWidth() - x0 * 2;

    bypassLed.setBounds (getWidth() - S (14 + 26), S (9), S (26), S (26));
    // Selector K-12 / K-14 / K-20 arriba, centrado
    const int kW = S (52), kGap = S (6);
    const int kStart = (getWidth() - (kW * 3 + kGap * 2)) / 2;
    for (int i = 0; i < 3; ++i)
        kButtons[i].setBounds (kStart + i * (kW + kGap), S (10), kW, S (24));

    // Medidores: IN a la izquierda, OUT a la derecha
    inMeter.setBounds (S (5), S (54), S (50), S (452));
    outMeter.setBounds (getWidth() - S (55), S (54), S (50), S (452));
    inTrimSlider.setBounds (S (57), S (54), S (50), S (452));
    trimSlider.setBounds (getWidth() - S (107), S (54), S (50), S (452));

    const int m = S (14), labelX = x0 + m + S (14), labelW = S (95), controlX = labelX + labelW + S (8);
    const int controlW = x0 + cw - controlX - (m + S (14)), rowH = S (26);

    const int gap = S (6), btnW = (cw - m * 2 - gap) / 2;
    linearButton.setBounds (x0 + m, S (54), btnW, S (28));
    adaptiveButton.setBounds (x0 + m + btnW + gap, S (54), btnW, S (28));

   int y = S (92 + 38);
    sampleRateRedLabel.setBounds (labelX, y, labelW, rowH);   sampleRateRedSlider.setBounds (controlX, y, controlW, rowH);
    y += S (32);
    sampleSizeLabel.setBounds (labelX, y, labelW, rowH);      sampleSizeSlider.setBounds (controlX, y, controlW, rowH);
    y += S (32);
    antiAliasLabel.setBounds (labelX, y, labelW, rowH);       antiAliasSlider.setBounds (controlX, y, controlW, rowH);

    const int rowEven = S (238 + 38), rowOdd = rowEven + S (32);
    evenLabel.setBounds (labelX, rowEven, labelW, rowH);   evenKnob.setBounds (controlX, rowEven, controlW, rowH);
    oddLabel.setBounds (labelX, rowOdd, labelW, rowH);     oddKnob.setBounds (controlX, rowOdd, controlW, rowH);

    const int top = S (352 + 34), startX = x0 + m + S (10), colW = (cw - (m + S (10)) * 2) / 3;
    const int knobW = S (86), knobH = S (92);
    auto place = [&] (int i, juce::Label& label, juce::Slider& knob)
    {
        label.setBounds (startX + colW * i, top, colW, S (18));
        knob.setBounds (startX + colW * i + (colW - knobW) / 2, top + S (18), knobW, knobH);
    };
    place (0, distortionLabel, distortionKnob);
    place (1, noiseLabel, noiseKnob);
    place (2, saturationLabel, saturationKnob);

}
