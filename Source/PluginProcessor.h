/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class LoFibyPAPIELGUAPOAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    LoFibyPAPIELGUAPOAudioProcessor();
    ~LoFibyPAPIELGUAPOAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Potencia media (RMS^2, ventana 300 ms) por canal L/R para los medidores K-System
    std::atomic<float> meterIn[2] { 0.0f, 0.0f };
    std::atomic<float> meterOut[2] { 0.0f, 0.0f };
    std::atomic<float> peakIn[2] { 0.0f, 0.0f };
    std::atomic<float> peakOut[2] { 0.0f, 0.0f };

private:
    void updateMeter (std::atomic<float>* dest, std::atomic<float>* peak, const juce::AudioBuffer<float>& buf, float* state);
    float meterStateIn[2] { 0.0f, 0.0f };
    float lastInGain = 1.0f;
    float meterStateOut[2] { 0.0f, 0.0f };
    double currentSampleRate { 44100.0 };
    float phase[2] { 0.0f, 0.0f };
    float lastSample[2] { 0.0f, 0.0f };
    float envelope[2] { 0.0f, 0.0f };
    float dcIn[2] { 0.0f, 0.0f };
    float dcOut[2] { 0.0f, 0.0f };
    float tapePink[2][3] {};
    float tapeHead[2] { 0.0f, 0.0f };
    float distDc[2] { 0.0f, 0.0f };
    float distLp[2] { 0.0f, 0.0f };

    juce::dsp::Oversampling<float> oversampling { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 8192 };
    juce::AudioBuffer<float> dryBuf;
    juce::SmoothedValue<float> smDist, smSat, smTape, smEven, smOdd, smTrim, smMix;
    std::array<std::vector<float>, 9> smBuf;
    std::atomic<float> *p_bypass {}, *p_quantMode {}, *p_sampleRateRed {}, *p_sampleSize {}, *p_antiAlias {},
                       *p_distortion {}, *p_saturation {}, *p_tape {}, *p_even {}, *p_odd {}, *p_trim {}, *p_inTrim {};
    void resetChannelState (int ch);
    float aaState[2][2] {};
    int maxBlock { 512 };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoFibyPAPIELGUAPOAudioProcessor)
};
