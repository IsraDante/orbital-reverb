#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::String percent (float v) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }

    float parseNumber (const juce::String& text) { return text.retainCharacters ("0123456789.-+").getFloatValue(); }

    juce::NormalisableRange<float> unitRange() { return { 0.0f, 1.0f, 0.0f }; }

    std::unique_ptr<juce::AudioParameterFloat> makeParam (const char* id, const char* name, float defaultValue,
                                                          std::function<juce::String (float, int)> toText,
                                                          std::function<float (const juce::String&)> fromText)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name, unitRange(), defaultValue,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction (std::move (toText))
                .withValueFromStringFunction ([fromText] (const juce::String& s) { return juce::jlimit (0.0f, 1.0f, fromText (s)); }));
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OrbitalReverbProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "planet", 1 }, "Planet",
        juce::StringArray { "Moon", "Mercury", "Jupiter", "Black Hole" }, 1));

    layout.add (makeParam ("size", "Size", 0.444f,
        [] (float v, int) { return juce::String (juce::roundToInt (60.0f + 90.0f * v)) + "%"; },
        [] (const juce::String& s) { return (parseNumber (s) - 60.0f) / 90.0f; }));

    layout.add (makeParam ("predelay", "Pre-delay", 0.15f,
        [] (float v, int) { return juce::String (juce::roundToInt (v * 200.0f)) + " ms"; },
        [] (const juce::String& s) { return parseNumber (s) / 200.0f; }));

    layout.add (makeParam ("tone", "Tone", 0.75f,
        [] (float v, int)
        {
            const float hz = orbital::toneHz (v);
            return hz >= 1000.0f ? juce::String (hz / 1000.0f, 1) + " kHz"
                                 : juce::String (juce::roundToInt (hz)) + " Hz";
        },
        [] (const juce::String& s)
        {
            float hz = parseNumber (s);
            if (s.containsIgnoreCase ("k")) hz *= 1000.0f;
            return std::log (juce::jmax (1.0f, hz) / 800.0f) / std::log (20.0f);
        }));

    layout.add (makeParam ("decay", "Decay", 0.5f,
        [] (float v, int) { return percent (v); },
        [] (const juce::String& s) { return parseNumber (s) / 100.0f; }));

    layout.add (makeParam ("mix", "Mix", 0.35f,
        [] (float v, int) { return percent (v); },
        [] (const juce::String& s) { return parseNumber (s) / 100.0f; }));

    layout.add (makeParam ("output", "Output", 0.75f,
        [] (float v, int)
        {
            if (v < 0.02f) return juce::String ("-inf dB");
            const float db = orbital::outputDb (v);
            return (db >= 0.0f ? "+" : "") + juce::String (db, 1) + " dB";
        },
        [] (const juce::String& s)
        {
            if (s.containsIgnoreCase ("inf")) return 0.0f;
            const float db = parseNumber (s);
            return db <= 0.0f ? 0.75f * (1.0f + db / 48.0f) : 0.75f + db / 24.0f;
        }));

    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return layout;
}

//==============================================================================
OrbitalReverbProcessor::OrbitalReverbProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "OrbitalReverb", createParameterLayout())
{
    planet   = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("planet"));
    size     = dynamic_cast<juce::AudioParameterFloat*>  (apvts.getParameter ("size"));
    preDelay = dynamic_cast<juce::AudioParameterFloat*>  (apvts.getParameter ("predelay"));
    tone     = dynamic_cast<juce::AudioParameterFloat*>  (apvts.getParameter ("tone"));
    decay    = dynamic_cast<juce::AudioParameterFloat*>  (apvts.getParameter ("decay"));
    mix      = dynamic_cast<juce::AudioParameterFloat*>  (apvts.getParameter ("mix"));
    output   = dynamic_cast<juce::AudioParameterFloat*>  (apvts.getParameter ("output"));
    bypass   = dynamic_cast<juce::AudioParameterBool*>   (apvts.getParameter ("bypass"));

    jassert (planet && size && preDelay && tone && decay && mix && output && bypass);

    startTimerHz (20);
}

OrbitalReverbProcessor::~OrbitalReverbProcessor()
{
    stopTimer();
}

//==============================================================================
bool OrbitalReverbProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    if (out == juce::AudioChannelSet::stereo())
        return in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();

    return out == juce::AudioChannelSet::mono() && in == juce::AudioChannelSet::mono();
}

void OrbitalReverbProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    maxBlockSize = juce::jmax (1, samplesPerBlock);

    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlockSize, 2 };

    {
        const juce::ScopedLock sl (irLock);
        convolution.prepare (spec);
        convolution.reset();
    }

    preDelayLine.prepare (spec);
    preDelayLine.setMaximumDelayInSamples ((int) std::ceil (0.2 * sampleRate) + 8);
    preDelayLine.reset();

    toneFilter.prepare (spec);
    toneFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    toneFilter.setResonance (1.122f); // Q = 1 dB, como el filtro del navegador
    toneFilter.reset();

    limiter.prepare (sampleRate);

    dryBuffer.setSize (2, maxBlockSize, false, false, true);
    wetBuffer.setSize (2, maxBlockSize, false, false, true);

    slowCoef = (float) std::exp (-1.0 / (0.03 * sampleRate));
    fastCoef = (float) std::exp (-1.0 / (0.01 * sampleRate));
    smoothersReady = false;

    refreshImpulseResponse (true);
}

//==============================================================================
juce::String OrbitalReverbProcessor::currentIRKey() const
{
    return juce::String (planet->getIndex()) + "|" + juce::String (size->get(), 2) + "|" + juce::String (decay->get(), 2);
}

void OrbitalReverbProcessor::refreshImpulseResponse (bool force)
{
    const juce::ScopedLock sl (irLock);
    const auto key = currentIRKey();
    if (! force && key == loadedIRKey)
        return;

    loadedIRKey = key;
    pendingIRKey = key;

    // Semilla fija por ajuste: mismos controles = misma reverb (importante para exportar en el DAW).
    juce::Random rng ((juce::int64) key.hashCode64());
    const double sr = currentSampleRate.load();

    auto ir = orbital::generateImpulseResponse (planet->getIndex(), size->get(), decay->get(), sr, rng);
    convolution.loadImpulseResponse (std::move (ir), sr,
                                     juce::dsp::Convolution::Stereo::yes,
                                     juce::dsp::Convolution::Trim::no,
                                     juce::dsp::Convolution::Normalise::no);
}

void OrbitalReverbProcessor::timerCallback()
{
    bool shouldRefresh = false;

    {
        const juce::ScopedLock sl (irLock);
        const auto key = currentIRKey();
        const auto now = juce::Time::getMillisecondCounter();

        if (key == loadedIRKey)
            pendingIRKey = key;
        else if (key != pendingIRKey)
        {
            pendingIRKey = key;
            pendingSince = now;
        }
        else
            shouldRefresh = now - pendingSince >= 120; // espera breve mientras giras el knob, como en el HTML
    }

    if (shouldRefresh)
        refreshImpulseResponse (false);
}

//==============================================================================
void OrbitalReverbProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numIn == 1 && numOut >= 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    const int numCh = juce::jmin (numOut, buffer.getNumChannels(), 2);
    if (numCh <= 0 || numSamples <= 0)
        return;

    const float sr = (float) currentSampleRate.load();

    // Valores objetivo (funcion apply del HTML)
    const float tDelay  = orbital::preDelaySeconds (preDelay->get()) * sr;
    const float tTone   = juce::jmin (orbital::toneHz (tone->get()), sr * 0.45f);
    const float tDry    = orbital::dryGain (mix->get());
    const float tWet    = orbital::wetGain (mix->get());
    const float tOut    = orbital::outputGain (output->get());
    const bool  isByp   = bypass->get();
    const float tActive = isByp ? 0.0f : 0.8f;
    const float tBypass = isByp ? 0.8f : 0.0f;

    if (! smoothersReady)
    {
        sDelay = tDelay; sTone = tTone; sDry = tDry; sWet = tWet; sOut = tOut; sActive = tActive; sBypass = tBypass;
        toneFilter.setCutoffFrequency (sTone);
        smoothersReady = true;
    }

    float peak = 0.0f;

    for (int start = 0; start < numSamples; start += maxBlockSize)
    {
        const int len = juce::jmin (maxBlockSize, numSamples - start);

        // 1) Copia seca + pre-delay hacia la senal de reverb
        for (int ch = 0; ch < numCh; ++ch)
            dryBuffer.copyFrom (ch, 0, buffer, ch, start, len);

        for (int i = 0; i < len; ++i)
        {
            sDelay = tDelay + slowCoef * (sDelay - tDelay);
            preDelayLine.setDelay (juce::jmax (0.0f, sDelay));

            for (int ch = 0; ch < numCh; ++ch)
            {
                preDelayLine.pushSample (ch, dryBuffer.getSample (ch, i));
                wetBuffer.setSample (ch, i, preDelayLine.popSample (ch));
            }
        }

        // 2) Convolucion (la reverb en si)
        {
            juce::dsp::AudioBlock<float> block (wetBuffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) len);
            juce::dsp::ProcessContextReplacing<float> context (block);
            convolution.process (context);
        }

        // 3) Filtro Tone, mezcla, limitador, bypass y salida
        for (int i = 0; i < len; ++i)
        {
            sTone   = tTone   + slowCoef * (sTone - tTone);
            sDry    = tDry    + slowCoef * (sDry - tDry);
            sWet    = tWet    + slowCoef * (sWet - tWet);
            sOut    = tOut    + slowCoef * (sOut - tOut);
            sActive = tActive + fastCoef * (sActive - tActive);
            sBypass = tBypass + fastCoef * (sBypass - tBypass);

            if ((i & 7) == 0)
                toneFilter.setCutoffFrequency (sTone);

            float frame[2] = { 0.0f, 0.0f };
            float dry[2]   = { 0.0f, 0.0f };

            for (int ch = 0; ch < numCh; ++ch)
            {
                dry[ch] = dryBuffer.getSample (ch, i);
                const float wet = toneFilter.processSample (ch, wetBuffer.getSample (ch, i));
                frame[ch] = (dry[ch] * sDry + wet * sWet) * sActive;
            }

            limiter.processFrame (frame, numCh);

            for (int ch = 0; ch < numCh; ++ch)
            {
                const float out = (frame[ch] + dry[ch] * sBypass) * sOut;
                buffer.setSample (ch, start + i, out);
                peak = juce::jmax (peak, std::abs (out));
            }
        }
    }

    if (numCh == 1 && numOut >= 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    float previous = outputPeak.load();
    while (peak > previous && ! outputPeak.compare_exchange_weak (previous, peak)) {}
}

//==============================================================================
juce::AudioProcessorEditor* OrbitalReverbProcessor::createEditor()
{
    return new OrbitalReverbEditor (*this);
}

void OrbitalReverbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void OrbitalReverbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrbitalReverbProcessor();
}
