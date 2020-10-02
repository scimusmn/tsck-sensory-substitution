#ifndef SMM_IMAGE_PLAYER_HPP
#define SMM_IMAGE_PLAYER_HPP

extern "C" {
    #include <portaudio.h>
}

#include <vector>
#include <opencv2/core.hpp>

#ifndef AUDIO_SAMPLE_RATE
#define AUDIO_SAMPLE_RATE 44100
#endif

#define PI 3.141592

class ImagePlayer
{
public:
    /* @brief (constructor)
     *
     * @param minimumFrequency Frequency to use for the lowest line.
     * @param maximumFrequency Frequency to use for the highest line.
     * @param horizontalLines How many frequency tracks to use when playing images.
     * @param timePerColumn Time in seconds to linger on each pixel column.
     */
    ImagePlayer(double minimumFrequency,
                double maximumFrequency,
                double volume,
                unsigned int horizontalLines,
                double timePerColumn);

    /* @brief Play an image as audio.
     *
     * @param image An OpenCV matrix containing the image to play.
     *
     * @returns true if the operation was successful; false if an error occurred.
     */
    bool play(cv::Mat image);

    static int audioCallback(const void* inputBuffer,
                             void* outputBuffer,
                             unsigned long frames,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags flags,
                             void* data);

    float getWaveForm();

private:
    double volume;
    double minFreq, maxFreq;
    double freqStep;
    unsigned int lines;
    double columnTime;
    double t;
    cv::Mat currentColumn;
    std::mutex columnMutex;
};

#endif
