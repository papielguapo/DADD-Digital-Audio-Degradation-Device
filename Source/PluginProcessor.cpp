/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout LoFibyPAPIELGUAPOAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "quantMode", 1 }, "Quantization Mode",
                                                                    juce::StringArray { "Linear", "Adaptive" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "sampleRateRed", 1 }, "Sample Rate Reduction",
                                                                   juce::NormalisableRange<float> (1000.0f, 48001.0f, 1.0f, 0.5f), 48001.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "bitDepth", 1 }, "Sample Size", 1, 24, 24));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "antiAlias", 1 }, "Anti-Alias",
                                                                   juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "distortion", 1 }, "Distortion",
                                                                   juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "tape", 1 }, "Tape",
                                                                   juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "saturation", 1 }, "Saturation",
                                                                   juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "even", 1 }, "Even (2nd)",
                                                                   juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "odd", 1 }, "Odd (3rd)",
                                                                   juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "inTrim", 1 }, "Input Trim",
                                                                   juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "trim", 1 }, "Output Trim",
                                                                   juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "kScale", 1 }, "K-System",
                                                                    juce::StringArray { "K-12", "K-14", "K-20" }, 0));

    return { params.begin(), params.end() };
}

//==============================================================================
LoFibyPAPIELGUAPOAudioProcessor::LoFibyPAPIELGUAPOAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    p_bypass = apvts.getRawParameterValue ("bypass");
    p_quantMode = apvts.getRawParameterValue ("quantMode");
    p_sampleRateRed = apvts.getRawParameterValue ("sampleRateRed");
    p_sampleSize = apvts.getRawParameterValue ("bitDepth");
    p_antiAlias = apvts.getRawParameterValue ("antiAlias");
    p_distortion = apvts.getRawParameterValue ("distortion");
    p_saturation = apvts.getRawParameterValue ("saturation");
    p_tape = apvts.getRawParameterValue ("tape");
    p_even = apvts.getRawParameterValue ("even");
    p_odd = apvts.getRawParameterValue ("odd");
    p_trim = apvts.getRawParameterValue ("trim");
    p_inTrim = apvts.getRawParameterValue ("inTrim");
}

void LoFibyPAPIELGUAPOAudioProcessor::resetChannelState (int ch)
{
    phase[ch] = 0.0f; lastSample[ch] = 0.0f; envelope[ch] = 0.0f;
    dcIn[ch] = 0.0f; dcOut[ch] = 0.0f; distDc[ch] = 0.0f; distLp[ch] = 0.0f;
    tapePink[ch][0] = tapePink[ch][1] = tapePink[ch][2] = 0.0f; tapeHead[ch] = 0.0f;
    aaState[ch][0] = aaState[ch][1] = 0.0f;
}

LoFibyPAPIELGUAPOAudioProcessor::~LoFibyPAPIELGUAPOAudioProcessor()
{
}

//==============================================================================
const juce::String LoFibyPAPIELGUAPOAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LoFibyPAPIELGUAPOAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LoFibyPAPIELGUAPOAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LoFibyPAPIELGUAPOAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LoFibyPAPIELGUAPOAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LoFibyPAPIELGUAPOAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int LoFibyPAPIELGUAPOAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LoFibyPAPIELGUAPOAudioProcessor::setCurrentProgram (int)
{
}

const juce::String LoFibyPAPIELGUAPOAudioProcessor::getProgramName (int)
{
    return {};
}

void LoFibyPAPIELGUAPOAudioProcessor::changeProgramName (int, const juce::String&)
{
}

//==============================================================================
void LoFibyPAPIELGUAPOAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    maxBlock = juce::jmax (1, samplesPerBlock);
    oversampling.initProcessing ((size_t) maxBlock);
    oversampling.reset();

    const int latency = juce::roundToInt (oversampling.getLatencyInSamples());
    setLatencySamples (latency);

    juce::dsp::ProcessSpec spec { currentSampleRate, (juce::uint32) maxBlock, 2 };
    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) latency);
    dryDelay.reset();

    dryBuf.setSize (2, maxBlock);
    for (auto& v : smBuf)
        v.assign ((size_t) maxBlock, 0.0f);

    auto initSm = [this] (juce::SmoothedValue<float>& s, float value)
    {
        s.reset (currentSampleRate, 0.03);
        s.setCurrentAndTargetValue (value);
    };
    initSm (smDist, 1.0f);
    initSm (smSat, 1.0f);
    initSm (smTape, 0.0f);
    initSm (smEven, 0.0f);
    initSm (smOdd, 0.0f);
    initSm (smTrim, 1.0f);
    smMix.reset (currentSampleRate, 0.01);
    smMix.setCurrentAndTargetValue (p_bypass->load() > 0.5f ? 0.0f : 1.0f);

    for (int ch = 0; ch < 2; ++ch)
    {
        aaState[ch][0] = aaState[ch][1] = 0.0f;
        distDc[ch] = distLp[ch] = 0.0f;
        phase[ch] = 0.0f;
        lastSample[ch] = 0.0f;
        envelope[ch] = 0.0f;
        dcIn[ch] = 0.0f;
        dcOut[ch] = 0.0f;
        tapePink[ch][0] = tapePink[ch][1] = tapePink[ch][2] = 0.0f;
        tapeHead[ch] = 0.0f;
    }
}

void LoFibyPAPIELGUAPOAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LoFibyPAPIELGUAPOAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void LoFibyPAPIELGUAPOAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ignoreUnused (midiMessages);
	juce::ScopedNoDenormals noDenormals;
	const int totalNumInputChannels  = getTotalNumInputChannels();
	const int totalNumOutputChannels = getTotalNumOutputChannels();
	const int numSamples = buffer.getNumSamples();

	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear (i, 0, numSamples);

	{
		const float g = juce::Decibels::decibelsToGain (p_inTrim->load());
		buffer.applyGainRamp (0, numSamples, lastInGain, g);
		lastInGain = g;
	}
	updateMeter (meterIn, peakIn, buffer, meterStateIn);

	const bool isBypassed     = p_bypass->load() > 0.5f;
	const int quantMode       = static_cast<int> (p_quantMode->load());
	const float sampleRateRed = p_sampleRateRed->load();
	const int sampleBits      = juce::jlimit (1, 24, juce::roundToInt (p_sampleSize->load()));
	const float antiAliasMix  = juce::jlimit (0.0f, 1.0f, p_antiAlias->load() / 100.0f);

	smDist.setTargetValue (juce::Decibels::decibelsToGain (p_distortion->load()));
	smSat.setTargetValue  (juce::Decibels::decibelsToGain (p_saturation->load()));
	smTape.setTargetValue (p_tape->load() / 100.0f);
	smEven.setTargetValue (p_even->load() / 100.0f);
	smOdd.setTargetValue  (p_odd->load() / 100.0f);
	smTrim.setTargetValue (juce::Decibels::decibelsToGain (p_trim->load()));
	smMix.setTargetValue  (isBypassed ? 0.0f : 1.0f);

	const int chs = juce::jmin (totalNumInputChannels, 2, buffer.getNumChannels());
	if (chs <= 0)
		return;

	
	const int bits = sampleBits;
	const float levels = std::pow (2.0f, static_cast<float> (bits) - 1.0f);

	const float fs  = (float) currentSampleRate;
	const float fs2 = fs * 2.0f;
	const float twoPi = juce::MathConstants<float>::twoPi;

	const bool srActive = sampleRateRed <= 48000.0f && sampleRateRed < fs;
	const float srStep = srActive ? sampleRateRed / fs : 1.0f;
	const float aaCutoff = juce::jmin (0.45f * sampleRateRed, 0.45f * fs);
	const float aaK = 1.0f - std::exp (-twoPi * aaCutoff / fs);

	const float envAttack  = std::exp (-1.0f / (0.001f * fs));
	const float envRelease = std::exp (-1.0f / (0.050f * fs));

	const float tapeHfK = 1.0f - std::exp (-twoPi * 17000.0f / fs2);
	const float tapeLfK = 1.0f - std::exp (-twoPi * 60.0f / fs2);
	const float tapeEmK = 1.0f - std::exp (-twoPi * 3000.0f / fs2);
	const float dcR     = 1.0f - twoPi * 20.0f / fs2;
	const float twoOverPi = 2.0f / juce::MathConstants<float>::pi;

	for (int start = 0; start < numSamples; start += maxBlock)
	{
		const int n = juce::jmin (maxBlock, numSamples - start);

		for (int i = 0; i < n; ++i)
		{
			smBuf[0][(size_t) i] = smDist.getNextValue();
			smBuf[1][(size_t) i] = smSat.getNextValue();
			smBuf[2][(size_t) i] = smTape.getNextValue();
			smBuf[3][(size_t) i] = smEven.getNextValue();
			smBuf[4][(size_t) i] = smOdd.getNextValue();
			smBuf[5][(size_t) i] = smTrim.getNextValue();
			smBuf[6][(size_t) i] = smMix.getNextValue();
			smBuf[7][(size_t) i] = std::pow (smBuf[0][(size_t) i], 0.6f);
			smBuf[8][(size_t) i] = std::pow (smBuf[1][(size_t) i], 0.55f);
		}

		// Etapa 1 (frecuencia base): copia seca retrasada, reduccion de sample rate con anti-alias real y cuantizacion
		for (int ch = 0; ch < chs; ++ch)
		{
			float* data = buffer.getWritePointer (ch) + start;
			float* dry  = dryBuf.getWritePointer (ch);

			for (int i = 0; i < n; ++i)
			{
				float in = data[i];
				if (! std::isfinite (in)) { in = 0.0f; resetChannelState (ch); }
				dryDelay.pushSample (ch, in);
				dry[i] = dryDelay.popSample (ch);

				float x = in;

				if (srActive)
				{
					float src = in;
					if (antiAliasMix > 0.0f)
					{
						aaState[ch][0] += aaK * (in - aaState[ch][0]);
						aaState[ch][1] += aaK * (aaState[ch][0] - aaState[ch][1]);
						src = in + antiAliasMix * (aaState[ch][1] - in);
					}

					phase[ch] += srStep;
					if (phase[ch] >= 1.0f)
					{
						phase[ch] -= std::floor (phase[ch]);
						lastSample[ch] = src;
					}
					x = lastSample[ch];
				}
				else
				{
					lastSample[ch] = in;
					phase[ch] = 0.0f;
					aaState[ch][0] = aaState[ch][1] = in;
				}

				if (bits < 24)
				{
					if (quantMode == 0)
					{
						x = std::floor (x * levels) / levels;
					}
					else
					{
						const float absX = std::abs (x);
						const float coeff = absX > envelope[ch] ? envAttack : envRelease;
						envelope[ch] = coeff * envelope[ch] + (1.0f - coeff) * absX;

						const float scale = juce::jmax (envelope[ch], absX, 0.0001f);
						x = std::round (x / scale * levels) / levels * scale;
					}
				}

				data[i] = x;
			}
		}

		// Etapa 2 (2x sobremuestreo): Distortion, Tape, Saturation y Harmonics
		juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) chs, (size_t) start, (size_t) n);
		auto up = oversampling.processSamplesUp (block);
		const size_t upN = up.getNumSamples();

		for (int ch = 0; ch < chs; ++ch)
		{
			float* d = up.getChannelPointer ((size_t) ch);
			auto& st = tapePink[ch];

			for (size_t u = 0; u < upN; ++u)
			{
				const size_t k = u >> 1;
				float x = d[u];

				const float g = smBuf[0][k];
				if (g > 1.0005f)
				{
					// Distortion: asimetria ligera (waveshaper con sesgo) + soft clip cubico a hard clip, tono suavizado
					const float amt = juce::jmin (1.0f, (g - 1.0f) * 20.0f);
					const float bias = 0.08f * juce::jmin (1.0f, (g - 1.0f) * 0.5f);
					const float u = juce::jlimit (-3.0f, 3.0f, x * g + bias);
					const float soft = std::tanh (u);
					const float hard = juce::jlimit (-1.0f, 1.0f, u * 0.9f);
					const float mixH = juce::jlimit (0.0f, 0.35f, (g - 2.0f) * 0.05f);
					float wet = soft + mixH * (hard - soft);

					distDc[ch] += 0.0005f * (wet - distDc[ch]);
					wet -= distDc[ch];
					distLp[ch] += 0.7f * (wet - distLp[ch]);
					wet = distLp[ch] / smBuf[7][k];

					x += amt * (wet - x);
				}

				const float a = smBuf[2][k];
				if (a > 0.0001f)
				{
					const float drive = 1.0f + 2.2f * a;
					const float bias  = 0.04f * a;
					const float bt    = std::tanh (bias);
					const float norm  = 1.0f / (drive * (1.0f - bt * bt));

					st[1] += tapeEmK * (x - st[1]);
					const float xin = x + 0.5f * a * (x - st[1]);

					float y = (std::tanh (xin * drive + bias) - bt) * norm;

					st[2] += tapeEmK * (y - st[2]);
					y -= 0.5f * a * (y - st[2]) / (1.0f + 0.5f * a);

					tapeHead[ch] += tapeHfK * (y - tapeHead[ch]);
					y += 0.4f * a * (tapeHead[ch] - y);

					st[0] += tapeLfK * (y - st[0]);
					y += 0.35f * a * st[0];

					x += juce::jmin (1.0f, a * 20.0f) * (y - x);
				}

				const float sg = smBuf[1][k];
				if (sg > 1.0005f)
				{
					// Saturation: mezcla de atan (suave) y curva algebraica x/(1+|x|) para compresion tipo valvula
					const float v = x * sg;
					const float a1 = twoOverPi * std::atan (v * 1.2f);
					const float a2 = v / (1.0f + std::abs (v));
					const float wet = (0.6f * a1 + 0.4f * a2) / smBuf[8][k];
					x += juce::jmin (1.0f, (sg - 1.0f) * 20.0f) * (wet - x);
				}

				const float evenAmt = smBuf[3][k];
				const float oddAmt  = smBuf[4][k];
				if (evenAmt > 0.0f || oddAmt > 0.0f)
				{
					const float c = juce::jlimit (-1.0f, 1.0f, x);
					const float even2 = 2.0f * c * c - 1.0f;
					const float odd3  = 4.0f * c * c * c - 3.0f * c;

					const float evenAc = even2 - dcIn[ch] + dcR * dcOut[ch];
					dcIn[ch] = even2;
					dcOut[ch] = evenAc;

					x += 0.5f * evenAmt * evenAc + 0.5f * oddAmt * odd3;
				}

				if (! std::isfinite (x)) { x = 0.0f; resetChannelState (ch); }
				d[u] = x;
			}
		}

		oversampling.processSamplesDown (block);

		// Mezcla bypass (crossfade con copia seca alineada) y trim
		for (int ch = 0; ch < chs; ++ch)
		{
			float* data = buffer.getWritePointer (ch) + start;
			const float* dry = dryBuf.getReadPointer (ch);

			for (int i = 0; i < n; ++i)
			{
				const float mix = smBuf[6][(size_t) i];
				const float tr = smBuf[5][(size_t) i];
				const float w = data[i] * tr;
				data[i] = dry[i] + mix * (w - dry[i]);
			}
		}
	}

	updateMeter (meterOut, peakOut, buffer, meterStateOut);
}

void LoFibyPAPIELGUAPOAudioProcessor::updateMeter (std::atomic<float>* dest, std::atomic<float>* peak, const juce::AudioBuffer<float>& buf, float* state)
{
	const int n = buf.getNumSamples();
	const int chs = buf.getNumChannels();
	if (n <= 0 || chs <= 0)
		return;

	// Potencia media con integracion de 300 ms, calculada por muestra (independiente del tamano de bloque)
	const float coef = std::exp (-1.0f / (0.3f * (float) currentSampleRate));

	for (int ch = 0; ch < 2; ++ch)
	{
		const float* d = buf.getReadPointer (juce::jmin (ch, chs - 1));
		float s = state[ch];
		float pk = 0.0f;

		for (int i = 0; i < n; ++i)
		{
			const float v = d[i];
			s = coef * s + (1.0f - coef) * v * v;
			pk = juce::jmax (pk, std::abs (v));

			// True peak aproximado: interpolacion cubica en el punto medio entre muestras
			if (i >= 1 && i + 1 < n)
			{
				const float mid = (-d[i - 1] + 9.0f * d[i] + 9.0f * d[i + 1] - (i + 2 < n ? d[i + 2] : d[i + 1])) * 0.0625f;
				pk = juce::jmax (pk, std::abs (mid));
			}
		}

		if (! std::isfinite (s))
			s = 0.0f;

		state[ch] = s;
		dest[ch].store (s);

		if (std::isfinite (pk) && pk > peak[ch].load (std::memory_order_relaxed))
			peak[ch].store (pk, std::memory_order_relaxed);
	}
}
//==============================================================================
bool LoFibyPAPIELGUAPOAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* LoFibyPAPIELGUAPOAudioProcessor::createEditor()
{
    return new LoFibyPAPIELGUAPOAudioProcessorEditor (*this);
}

//==============================================================================
void LoFibyPAPIELGUAPOAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
		copyXmlToBinary (*xml, destData);
}

void LoFibyPAPIELGUAPOAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        auto vt = juce::ValueTree::fromXml (*xmlState);

        for (int i = 0; i < vt.getNumChildren(); ++i)
        {
            auto child = vt.getChild (i);
            const auto id = child.getProperty ("id").toString();

            if (id == "sampleSize")
            {
                static constexpr int oldBits[5] = { 24, 16, 8, 4, 2 };
                child.setProperty ("id", "bitDepth", nullptr);
                child.setProperty ("value", oldBits[juce::jlimit (0, 4, juce::roundToInt ((float) child.getProperty ("value")))], nullptr);
            }
            else if (id == "noise")
                child.setProperty ("id", "tape", nullptr);
            else if (id == "sampleRateRed" && ((double) child.getProperty ("value") == 44100.0 || (double) child.getProperty ("value") == 50000.0))
                child.setProperty ("value", 48001.0, nullptr);
        }

        apvts.replaceState (vt);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LoFibyPAPIELGUAPOAudioProcessor();
}
