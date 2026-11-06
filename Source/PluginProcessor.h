#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "ReverbEngine.h"

class OrbitalReverbProcessor final : public juce::AudioProcessor,
                                     private juce::Timer
{
public:
    OrbitalReverbProcessor();
    ~OrbitalReverbProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Orbital Reverb"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 7.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

    /** Pico de salida desde el ultimo refresco de pantalla (para el medidor y el LED). */
    std::atomic<float> outputPeak { 0.0f };

    /** Regenera la reverb si cambian Planet, Size o Decay. */
    void refreshImpulseResponse (bool force);
    int getCurrentIRSize() const { return convolution.getCurrentIRSize(); }

private:
    void timerCallback() override;
    juce::String currentIRKey() const;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioParameterChoice* planet = nullptr;
    juce::AudioParameterFloat* size = nullptr;
    juce::AudioParameterFloat* preDelay = nullptr;
    juce::AudioParameterFloat* tone = nullptr;
    juce::AudioParameterFloat* decay = nullptr;
    juce::AudioParameterFloat* mix = nullptr;
    juce::AudioParameterFloat* output = nullptr;
    juce::AudioParameterBool* bypass = nullptr;

    juce::dsp::Convolution convolution;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> preDelayLine { 1 << 16 };
    juce::dsp::StateVariableTPTFilter<float> toneFilter;
    orbital::OutputLimiter limiter;

    juce::AudioBuffer<float> dryBuffer, wetBuffer;
    std::atomic<double> currentSampleRate { 44100.0 };
    int maxBlockSize = 512;

    // Suavizado de parametros (como setTargetAtTime del HTML)
    float slowCoef = 0.0f, fastCoef = 0.0f;
    float sDelay = 0.0f, sTone = 12000.0f, sDry = 1.0f, sWet = 0.0f, sOut = 1.0f, sActive = 0.8f, sBypass = 0.0f;
    bool smoothersReady = false;

    // Protege la carga de la reverb: el DAW puede reiniciar el audio mientras se gira un knob.
    // Nunca se usa en el hilo de audio.
    juce::CriticalSection irLock;
    juce::String loadedIRKey, pendingIRKey;
    juce::uint32 pendingSince = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitalReverbProcessor)
};
