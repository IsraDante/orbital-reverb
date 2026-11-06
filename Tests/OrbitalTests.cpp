// Pruebas automaticas de Orbital Reverb: comprueban que el plugin se comporta como el HTML.
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <iostream>

static int failures = 0;

static void check (bool ok, const juce::String& what)
{
    std::cout << (ok ? "  OK    " : "  FALLO ") << what << std::endl;
    if (! ok) ++failures;
}

static bool near (double a, double b, double tol) { return std::abs (a - b) <= tol; }

static void setParam (OrbitalReverbProcessor& p, const char* id, float value01)
{
    p.apvts.getParameter (id)->setValueNotifyingHost (value01);
}

static void processSeconds (OrbitalReverbProcessor& p, double seconds, std::function<float (int)> input,
                            float& peakOut, double& energyOut, double sr = 48000.0, int skip = 0)
{
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    const int total = (int) (seconds * sr);
    peakOut = 0.0f; energyOut = 0.0;
    for (int pos = 0; pos < total; pos += 512)
    {
        for (int i = 0; i < 512; ++i)
            for (int ch = 0; ch < 2; ++ch)
                buf.setSample (ch, i, input (pos + i));
        p.processBlock (buf, midi);
        for (int i = 0; i < 512; ++i)
        {
            if (pos + i < skip) continue;
            for (int ch = 0; ch < 2; ++ch)
            {
                const float s = buf.getSample (ch, i);
                if (! std::isfinite (s)) { peakOut = 1.0e9f; return; }
                peakOut = std::max (peakOut, std::abs (s));
                energyOut += (double) s * s;
            }
        }
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "Conversiones de los controles (iguales al HTML):" << std::endl;
    check (near (orbital::outputDb (0.75f), 0.0, 1e-5), "Output 0.75 = 0 dB");
    check (near (orbital::outputDb (1.0f), 6.0, 1e-5), "Output al maximo = +6 dB");
    check (orbital::outputGain (0.01f) == 0.0f, "Output al minimo = silencio");
    check (near (orbital::toneHz (0.0f), 800.0, 0.01), "Tone minimo = 800 Hz");
    check (near (orbital::toneHz (1.0f), 16000.0, 0.5), "Tone maximo = 16 kHz");
    check (near (orbital::preDelaySeconds (1.0f), 0.2, 1e-6), "Pre-delay maximo = 200 ms");
    check (near (orbital::irLengthSeconds (3, 1.0f), 7.0, 1e-9), "Black Hole a tamano maximo = 7 s (limite)");
    check (near (orbital::irLengthSeconds (0, 0.444f), 0.5 * (0.6 + 0.9 * 0.444), 1e-6), "Longitud de Moon");
    check (near (orbital::dryGain (0.0f), 1.0, 1e-6) && near (orbital::wetGain (0.0f), 0.0, 1e-6), "Mix 0% = solo senal seca");

    {
        juce::Random r (1);
        auto ir = orbital::generateImpulseResponse (1, 0.444f, 0.5f, 48000.0, r);
        check (ir.getNumChannels() == 2 && ir.getNumSamples() == (int) std::floor (orbital::irLengthSeconds (1, 0.444f) * 48000.0),
               "Respuesta de reverb estereo con la longitud correcta");
    }

    std::cout << "Procesado de audio:" << std::endl;
    OrbitalReverbProcessor p;
    p.setPlayConfigDetails (2, 2, 48000.0, 512);
    p.prepareToPlay (48000.0, 512);

    {
        juce::AudioBuffer<float> silence (2, 512);
        juce::MidiBuffer midi;
        for (int i = 0; i < 400 && p.getCurrentIRSize() <= 1; ++i)
        {
            silence.clear();
            p.processBlock (silence, midi);
            juce::Thread::sleep (5);
        }
    }
    check (p.getCurrentIRSize() > 1, "La reverb se carga al iniciar");

    float peak; double energy;
    processSeconds (p, 3.0, [] (int i) { return i == 0 ? 1.0f : 0.0f; }, peak, energy, 48000.0, 4800);
    check (peak < 2.0f, "Sin valores no validos ni picos descontrolados");
    check (energy > 1.0e-4, "Hay cola de reverb despues de un golpe (energia " + juce::String (energy) + ")");

    // Mix 0%: solo senal seca -> 0.8 (master) x compensacion del limitador
    setParam (p, "mix", 0.0f);
    processSeconds (p, 8.0, [] (int) { return 0.0f; }, peak, energy); // vaciar la cola
    processSeconds (p, 0.5, [] (int i) { return 0.5f * std::sin (2.0 * juce::MathConstants<double>::pi * 440.0 * i / 48000.0); },
                    peak, energy, 48000.0, 12000);
    const float makeup = juce::Decibels::decibelsToGain (-0.6f * orbital::OutputLimiter::curveDb (0.0f));
    check (near (peak, 0.5 * 0.8 * makeup, 0.01), "Mix 0%: nivel de salida igual que en el HTML (" + juce::String (peak, 4) + ")");

    // Bypass: entrada x 0.8, como el boton del HTML
    setParam (p, "bypass", 1.0f);
    processSeconds (p, 0.5, [] (int i) { return 0.5f * std::sin (2.0 * juce::MathConstants<double>::pi * 440.0 * i / 48000.0); },
                    peak, energy, 48000.0, 12000);
    check (near (peak, 0.4, 0.01), "Bypass: salida = entrada x 0.8 (" + juce::String (peak, 4) + ")");

    // Limitador: una senal muy fuerte queda contenida
    setParam (p, "bypass", 0.0f);
    setParam (p, "mix", 0.35f);
    processSeconds (p, 1.0, [] (int i) { return 2.0f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 220.0 * i / 48000.0); },
                    peak, energy, 48000.0, 24000);
    check (peak < 1.3f, "Limitador: senal +6 dB queda controlada (" + juce::String (peak, 3) + ")");

    // Cambiar de planeta regenera la reverb
    const int before = p.getCurrentIRSize();
    setParam (p, "planet", 1.0f); // Black Hole
    p.refreshImpulseResponse (false);
    {
        juce::AudioBuffer<float> silence (2, 512);
        juce::MidiBuffer midi;
        for (int i = 0; i < 400 && p.getCurrentIRSize() == before; ++i)
        {
            silence.clear();
            p.processBlock (silence, midi);
            juce::Thread::sleep (5);
        }
    }
    check (p.getCurrentIRSize() > before, "Black Hole tiene una cola mas larga");

    // Guardar/cargar el estado (proyecto del DAW)
    {
        setParam (p, "size", 0.9f);
        juce::MemoryBlock state;
        p.getStateInformation (state);
        OrbitalReverbProcessor q;
        q.setStateInformation (state.getData(), (int) state.getSize());
        check (near (q.apvts.getRawParameterValue ("size")->load(), 0.9, 0.001), "El proyecto guarda y recupera los ajustes");
    }

    std::cout << "Interfaz:" << std::endl;
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
        editor->setSize (OrbitalPanel::baseWidth, OrbitalPanel::baseHeight);
        auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);
        juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile ("editor.png"));
        out.deleteFile();
        juce::FileOutputStream os (out);
        juce::PNGImageFormat().writeImageToStream (img, os);
        check (img.isValid(), "Captura de la interfaz: " + out.getFullPathName());
    }

    std::cout << (failures == 0 ? "\nTODO CORRECTO" : "\nHAY FALLOS: " + juce::String (failures).toStdString()) << std::endl;
    return failures == 0 ? 0 : 1;
}
