#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

/*
    Orbital Reverb - motor de sonido.
    Todo lo que hay aqui es una traduccion directa del HTML original
    (Orbital_Reverb.html), para que el plugin suene igual que en el navegador.
*/
namespace orbital
{
    constexpr float pi = 3.14159265358979f;

    //==========================================================================
    // Tipos de "planeta" (igual que la tabla TYPES del HTML)
    struct PlanetType
    {
        const char* name;
        float len;   // longitud base de la cola (segundos)
        float pw;    // curva de caida
        float br;    // brillo del ruido
        float sw;    // "swell" (entrada lenta), en segundos
        bool  er;    // reflexiones tempranas
    };

    inline const std::array<PlanetType, 4>& planetTypes()
    {
        static const std::array<PlanetType, 4> types { {
            { "Moon",       0.5f, 3.2f, 0.75f, 0.00f, true  },
            { "Mercury",    1.6f, 2.6f, 0.60f, 0.00f, false },
            { "Jupiter",    3.6f, 2.1f, 0.40f, 0.08f, false },
            { "Black Hole", 6.5f, 1.7f, 0.30f, 0.50f, false },
        } };
        return types;
    }

    //==========================================================================
    // Conversiones de los controles (0..1) a valores reales, igual que el HTML
    inline float outputDb (float n)        { return n < 0.75f ? -48.0f * (1.0f - n / 0.75f) : 24.0f * (n - 0.75f); }
    inline float outputGain (float n)      { return n < 0.02f ? 0.0f : std::pow (10.0f, outputDb (n) / 20.0f); }
    inline float toneHz (float n)          { return 800.0f * std::pow (20.0f, n); }
    inline float preDelaySeconds (float n) { return n * 0.2f; }
    inline float dryGain (float mix)       { return std::cos (mix * pi * 0.5f); }
    inline float wetGain (float mix)       { return std::sin (mix * pi * 0.5f) * 1.4f; }

    inline double irLengthSeconds (int planet, float size)
    {
        const auto& t = planetTypes()[(size_t) juce::jlimit (0, 3, planet)];
        return std::min (7.0, (double) t.len * (0.6 + 0.9 * (double) size));
    }

    //==========================================================================
    // Normalizacion identica a la del ConvolverNode del navegador (Web Audio),
    // para que el volumen de la reverb sea el mismo que en el HTML.
    inline float webAudioNormalisationScale (const juce::AudioBuffer<float>& ir, double sampleRate)
    {
        double power = 0.0;
        for (int c = 0; c < ir.getNumChannels(); ++c)
        {
            const float* d = ir.getReadPointer (c);
            for (int i = 0; i < ir.getNumSamples(); ++i)
                power += (double) d[i] * (double) d[i];
        }

        const double count = (double) ir.getNumChannels() * (double) ir.getNumSamples();
        power = count > 0.0 ? std::sqrt (power / count) : 0.0;

        constexpr double minPower = 0.000125;
        if (! std::isfinite (power) || power < minPower)
            power = minPower;

        double scale = 1.0 / power;
        scale *= std::pow (10.0, -58.0 / 20.0);   // calibracion de Web Audio (-58 dB)
        scale *= 44100.0 / sampleRate;
        return (float) scale;
    }

    //==========================================================================
    // Genera la respuesta al impulso de la reverb (funcion genIR del HTML)
    inline juce::AudioBuffer<float> generateImpulseResponse (int planet, float size, float decay,
                                                             double sampleRate, juce::Random& rng)
    {
        const auto& t = planetTypes()[(size_t) juce::jlimit (0, 3, planet)];
        const int length = juce::jmax (1, (int) std::floor (irLengthSeconds (planet, size) * sampleRate));
        const double curve = (double) t.pw * std::pow (3.0, 1.0 - 2.0 * (double) decay);

        juce::AudioBuffer<float> ir (2, length);

        for (int c = 0; c < 2; ++c)
        {
            float* d = ir.getWritePointer (c);
            double y = 0.0;

            for (int i = 0; i < length; ++i)
            {
                const double pos = (double) i / (double) length;
                const double a = std::max (0.04, (double) t.br * (1.0 - 0.75 * pos));
                const double w = t.sw > 0.0f ? std::min (1.0, (double) i / ((double) t.sw * sampleRate)) : 1.0;
                const double noise = rng.nextDouble() * 2.0 - 1.0;
                y += a * (noise - y);
                d[i] = (float) (y * std::pow (1.0 - pos, curve) * w);
            }

            if (t.er)
            {
                for (int k = 0; k < 8; ++k)
                {
                    const int p = (int) std::floor ((0.004 + k * 0.007 + rng.nextDouble() * 0.004) * sampleRate);
                    if (p < length)
                        d[p] += (float) (0.9 - k * 0.08) * (rng.nextBool() ? 1.0f : -1.0f);
                }
            }
        }

        ir.applyGain (webAudioNormalisationScale (ir, sampleRate));
        return ir;
    }

    //==========================================================================
    // Limitador de salida (equivale al DynamicsCompressor del HTML:
    // threshold -1 dB, knee 2 dB, ratio 12, attack 3 ms, release 100 ms)
    class OutputLimiter
    {
    public:
        static constexpr float thresholdDb = -1.0f;
        static constexpr float kneeDb      = 2.0f;
        static constexpr float ratio       = 12.0f;

        static float curveDb (float x)
        {
            const float over = x - thresholdDb;
            if (2.0f * over < -kneeDb)
                return x;
            if (2.0f * std::abs (over) <= kneeDb)
            {
                const float k = over + kneeDb * 0.5f;
                return x + (1.0f / ratio - 1.0f) * k * k / (2.0f * kneeDb);
            }
            return thresholdDb + over / ratio;
        }

        void prepare (double sampleRate)
        {
            attackCoef  = (float) std::exp (-1.0 / (0.003 * sampleRate));
            releaseCoef = (float) std::exp (-1.0 / (0.100 * sampleRate));
            makeup = juce::Decibels::decibelsToGain (-0.6f * curveDb (0.0f)); // ganancia de compensacion del navegador
            envelopeDb = 0.0f;
        }

        void processFrame (float* frame, int numChannels)
        {
            float peak = 0.0f;
            for (int c = 0; c < numChannels; ++c)
                peak = std::max (peak, std::abs (frame[c]));

            const float inDb = juce::Decibels::gainToDecibels (peak, -120.0f);
            const float reduction = std::max (0.0f, inDb - curveDb (inDb));
            const float coef = reduction > envelopeDb ? attackCoef : releaseCoef;
            envelopeDb = reduction + coef * (envelopeDb - reduction);

            const float g = juce::Decibels::decibelsToGain (-envelopeDb) * makeup;
            for (int c = 0; c < numChannels; ++c)
                frame[c] *= g;
        }

        float getMakeupGain() const { return makeup; }

    private:
        float attackCoef = 0.0f, releaseCoef = 0.0f, makeup = 1.0f, envelopeDb = 0.0f;
    };
}
