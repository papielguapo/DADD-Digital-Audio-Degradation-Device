#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class LoFiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static juce::Colour accent()  { return juce::Colour (0xff8fd14f); }
    static juce::Colour darkBg()  { return juce::Colour (0xff141618); }
    static juce::Colour outline() { return juce::Colour (0xff2e3438); }

    LoFiLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId,       juce::Colour (0xffe6e9ec));
        setColour (juce::Slider::textBoxBackgroundColourId, darkBg());
        setColour (juce::Slider::textBoxOutlineColourId,    outline());
        setColour (juce::TextButton::buttonColourId,        juce::Colour (0xff2a2e32));
        setColour (juce::TextButton::buttonOnColourId,      accent());
        setColour (juce::TextButton::textColourOffId,       juce::Colour (0xffb8c0c8));
        setColour (juce::TextButton::textColourOnId,        juce::Colour (0xff141618));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool) override
    {
        auto area = b.getLocalBounds().toFloat().reduced (3.0f);
        const float d = juce::jmin (area.getWidth(), area.getHeight());
        auto led = juce::Rectangle<float> (d, d).withCentre (area.getCentre());

        const bool on = b.getToggleState();
        if (on)
        {
            g.setColour (accent().withAlpha (0.25f));
            g.fillEllipse (led.expanded (2.0f));
        }
        g.setColour (on ? accent() : juce::Colour (0xff3a4046));
        g.fillEllipse (led);
        g.setColour (juce::Colour (0xff0d0f10));
        g.drawEllipse (led, 1.5f);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float, float,
                           const juce::Slider::SliderStyle style, juce::Slider&) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            const float trackW = 6.0f;
            juce::Rectangle<float> vt ((float) x + (float) width * 0.5f - trackW * 0.5f, (float) y, trackW, (float) height);
            const float pos = juce::jlimit ((float) y, (float) (y + height), sliderPos);
            g.setColour (juce::Colour (0xff0e1011));
            g.fillRoundedRectangle (vt, 3.0f);
            g.setColour (accent());
            g.fillRoundedRectangle (vt.withTop (pos), 3.0f);
            g.setColour (juce::Colour (0xffe5e9f0));
            g.fillEllipse (vt.getCentreX() - 7.0f, pos - 7.0f, 14.0f, 14.0f);
            return;
        }

        const float trackH = 6.0f;
        juce::Rectangle<float> track ((float) x, (float) y + (float) height * 0.5f - trackH * 0.5f, (float) width, trackH);

        g.setColour (juce::Colour (0xff0e1011));
        g.fillRoundedRectangle (track, 3.0f);
        g.setColour (accent());
        g.fillRoundedRectangle (track.withRight (juce::jlimit ((float) x, (float) (x + width), sliderPos)), 3.0f);
        g.setColour (juce::Colour (0xffe5e9f0));
        g.fillEllipse (sliderPos - 7.0f, track.getCentreY() - 7.0f, 14.0f, 14.0f);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float pos, float startAngle, float endAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto angle = startAngle + pos * (endAngle - startAngle);
        auto arcR = radius - 2.0f;

        g.setColour (juce::Colour (0xff0a0b0c));
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour (juce::Colour (0xff1a1d1f));
        g.fillEllipse (centre.x - radius + 5.0f, centre.y - radius + 5.0f, radius * 2.0f - 10.0f, radius * 2.0f - 10.0f);

        juce::Path bg, val;
        bg.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        val.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (juce::Colour (0xff32373d));
        g.strokePath (bg, juce::PathStrokeType (3.0f));
        g.setColour (accent());
        g.strokePath (val, juce::PathStrokeType (3.0f));

        juce::Path pointer;
        pointer.addRoundedRectangle (-1.25f, -radius + 8.0f, 2.5f, radius * 0.45f, 1.0f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (juce::Colours::white);
        g.fillPath (pointer);
    }
};

//==============================================================================
// Medidor estereo L/R estilo Bob Katz (K-System). Los niveles llegan en dB relativos a 0 K.
class KMeter : public juce::Component
{
public:
    explicit KMeter (const juce::String& titleText) : title (titleText) {}

    void setLevels (float l, float r, float pl, float pr)
    {
        levelL = l; updateHold (holdL, holdCountL, pl);
        levelR = r; updateHold (holdR, holdCountR, pr);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff222629));
        g.fillRoundedRectangle (b, 5.0f);
        g.setColour (juce::Colour (0xff2e3438));
        g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);

        const float u = b.getWidth() / 56.0f;
        g.setColour (juce::Colour (0xffe6e9ec));
        g.setFont (juce::FontOptions (13.0f * u, juce::Font::bold));
        g.drawText (title, b.removeFromTop (24.0f * u).toNearestInt(), juce::Justification::centred, false);

        auto labelArea = b.removeFromBottom (20.0f * u);
        auto area = b.reduced (6.0f * u, 4.0f * u);

        const float barW = 16.0f * u, scaleW = area.getWidth() - 2.0f * barW;
        auto leftBar  = area.removeFromLeft (barW);
        auto rightBar = area.removeFromRight (barW);
        auto scaleArea = area.withWidth (scaleW);

        drawBar (g, leftBar, levelL, holdL);
        drawBar (g, rightBar, levelR, holdR);

        g.setFont (juce::FontOptions (9.0f * u));
        for (int db = (int) minDb; db <= (int) maxDb; db += 4)
        {
            const float y = yFor ((float) db, leftBar);
            g.setColour (db == 0 ? juce::Colour (0xffe6e9ec) : juce::Colour (0xff7d868e));
            g.drawText (juce::String (db), juce::Rectangle<float> (scaleArea.getX() - 4.0f * u, y - 6.0f * u,
                                                                    scaleArea.getWidth() + 8.0f * u, 12.0f * u).toNearestInt(),
                        juce::Justification::centred, false);
        }

        g.setColour (juce::Colour (0xffb8c0c8));
        g.setFont (juce::FontOptions (11.0f * u, juce::Font::bold));
        g.drawText ("L", labelArea.removeFromLeft (labelArea.getWidth() * 0.5f).toNearestInt(), juce::Justification::centred, false);
        g.drawText ("R", labelArea.toNearestInt(), juce::Justification::centred, false);
    }

private:
    static constexpr float minDb = -32.0f;
    static constexpr float maxDb = 12.0f;

    static void updateHold (float& hold, int& count, float p)
    {
        if (p >= hold) { hold = p; count = 60; }
        else if (count > 0) --count;
        else hold = juce::jmax (minDb - 1.0f, hold - 0.6f);
    }

    static float yFor (float db, juce::Rectangle<float> r)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
        return r.getBottom() - t * r.getHeight();
    }

    static void drawBar (juce::Graphics& g, juce::Rectangle<float> r, float level, float hold)
    {
        g.setColour (juce::Colour (0xff0e1011));
        g.fillRoundedRectangle (r, 2.0f);

        const float top = yFor (level, r);
        const float yZero = yFor (0.0f, r);
        const float yRed = yFor (4.0f, r);

        auto fill = [&] (float yFrom, float yTo, juce::Colour c)
        {
            const float a = juce::jmax (yFrom, r.getY()), bb = juce::jmin (yTo, r.getBottom());
            if (bb > a)
            {
                g.setColour (c);
                g.fillRect (juce::Rectangle<float> (r.getX(), a, r.getWidth(), bb - a));
            }
        };

        fill (juce::jmax (top, yZero), r.getBottom(), juce::Colour (0xff8fd14f));   // verde: hasta 0 K
        fill (juce::jmax (top, yRed), yZero, juce::Colour (0xffe8c43a));            // amarillo: 0 a +4
        fill (top, yRed, juce::Colour (0xffe5484d));

        if (hold > minDb)
        {
            g.setColour (hold > 0.0f ? juce::Colour (0xffe5484d) : juce::Colour (0xffe6e9ec));
            g.fillRect (juce::Rectangle<float> (r.getX(), yFor (hold, r) - 1.0f, r.getWidth(), 2.0f));
        }                                // rojo: por encima de +4
    }

    juce::String title;
    float levelL { -100.0f }, levelR { -100.0f };
    float holdL { -100.0f }, holdR { -100.0f };
    int holdCountL { 0 }, holdCountR { 0 };
};

//==============================================================================
class LoFibyPAPIELGUAPOAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    LoFibyPAPIELGUAPOAudioProcessorEditor (LoFibyPAPIELGUAPOAudioProcessor&);
    ~LoFibyPAPIELGUAPOAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    KMeter inMeter { "IN" }, outMeter { "OUT" };
    juce::TextButton kButtons[3];
    int kRef = 20;
    std::unique_ptr<juce::ParameterAttachment> kAttachment;
    LoFibyPAPIELGUAPOAudioProcessor& audioProcessor;
    LoFiLookAndFeel customLookAndFeel;

    // LED de bypass: encendido = efecto activo
    juce::ToggleButton bypassLed;

    juce::TextButton linearButton { "Linear" }, adaptiveButton { "Adaptive" };

    juce::Label sampleRateRedLabel, sampleSizeLabel, antiAliasLabel;
    juce::Label distortionLabel, noiseLabel, saturationLabel, trimLabel, inTrimLabel, evenLabel, oddLabel;
    juce::Slider sampleRateRedSlider, sampleSizeSlider, antiAliasSlider, trimSlider, inTrimSlider;
    juce::Slider distortionKnob, noiseKnob, saturationKnob, evenKnob, oddKnob;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<juce::ParameterAttachment> bypassAttachment, quantAttachment;
    std::unique_ptr<SliderAttachment> sampleRateRedAttachment, antiAliasAttachment,
                                      distortionAttachment, noiseAttachment, evenAttachment, oddAttachment,
                                      saturationAttachment, trimAttachment, inTrimAttachment, bitsAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoFibyPAPIELGUAPOAudioProcessorEditor)
};

