#ifndef SMM_AUDIO_INDICATOR_H
#define SMM_AUDIO_INDICATOR_H

extern "C" {
#include <portaudio.h>
#include <math.h>
}

#define SAMPLE_RATE 44100
#define PI 3.141592
#define LN2 0.69314718056

static int sine_callback(const void* inputBuffer,
                         void* outputBuffer,
                         unsigned long frames,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags flags,
                         void* data);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class AudioIndicator {
private:
    float m_minFrequency;
    float m_maxFrequency;

    float m_minAmplitudeFrequency;
    float m_maxAmplitudeFrequency;
    
    float m_frequency;
    unsigned int m_samples;
    unsigned int m_index;

    float m_amplitude;
    float m_amplitudeFrequency;
    unsigned int m_amplitudeSamples;
    unsigned int m_amplitudeIndex;
    
    void recalculate() {
        float fractionalTime = (float) m_index / m_samples;
        m_samples = SAMPLE_RATE/m_frequency;
        m_index = fractionalTime * m_samples;

        fractionalTime = (float) m_amplitudeIndex / m_amplitudeSamples;
        m_amplitudeSamples = SAMPLE_RATE/m_amplitudeFrequency;
        m_amplitudeIndex = fractionalTime * m_amplitudeSamples;
    }

public:
    AudioIndicator(float minFrequency, float maxFrequency,
                   float minAmplitudeFrequency, float maxAmplitudeFrequency) {
        m_minFrequency = minFrequency;
        m_maxFrequency = maxFrequency;

        m_minAmplitudeFrequency = minAmplitudeFrequency;
        m_maxAmplitudeFrequency = maxAmplitudeFrequency;

        m_frequency = minFrequency;
        m_amplitude = 0.1;
        m_amplitudeFrequency = 10;
        m_amplitudeIndex = 0;
        m_amplitudeSamples = 1;
        m_samples = 1;
        m_index = 0;

        recalculate();
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    ~AudioIndicator() {
	
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    float waveLevel() {
        float frequencyPhase = (2*PI*m_index) / m_samples;
        float amplitudePhase = (2*PI*m_amplitudeIndex) / m_amplitudeSamples;
        m_index++;
        m_amplitudeIndex++;
        if (m_index > m_samples) {
            m_index = 0;
        }
        if (m_amplitudeIndex > m_amplitudeSamples) {
            m_amplitudeIndex = 0;
        }

        return 0.5 * m_amplitude * (sin(amplitudePhase)+1) * sin(frequencyPhase);
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    void setVolume(float volume) {
        m_amplitude = volume;
    }

    float update(float x, float y) {
        m_frequency = (log(x+1)/LN2) * (m_maxFrequency - m_minFrequency) + m_minFrequency;
        m_amplitudeFrequency = y
            * (m_maxAmplitudeFrequency - m_minAmplitudeFrequency)
            + m_minAmplitudeFrequency;
        recalculate();
    }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static int sine_callback(const void* inputBuffer,
                         void* outputBuffer,
                         unsigned long frames,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags flags,
                         void* data) {
    float* out = (float*) outputBuffer;
    AudioIndicator* ai = (AudioIndicator*) data;

    for (int i=0; i<frames; i++) {
        float level = ai->waveLevel();
        *out++ = level; *out++ = level;
    }

    return 0;
}
  
#endif
