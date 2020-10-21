#include <chrono>
#include <iostream>
#include <cmath>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "ImagePlayer.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ImagePlayer::ImagePlayer(double minimumFrequency,
                         double maximumFrequency,
                         double volume,
                         unsigned int horizontalLines,
                         double timePerColumn)
    : minFreq(minimumFrequency),
      maxFreq(maximumFrequency),
      volume(volume),
      lines(horizontalLines),
      columnTime(timePerColumn),
      t(0)
{
    freqStep = (maxFreq - minFreq)/lines;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool ImagePlayer::play(cv::Mat image)
{
    // scale image so that it has [lines] rows, and convert to grayscale.
    double scaling = double(lines)/image.rows;
    int resizedRows = lines;
    int resizedCols = scaling * image.cols;
    cv::Size newSize(resizedCols, resizedRows);
    cv::Mat scaledImage;
    cv::resize(image, scaledImage, newSize, 0, 0, cv::INTER_AREA);
    if (scaledImage.type() != CV_8U) {
	cv::cvtColor(scaledImage, scaledImage, cv::COLOR_BGR2GRAY);
    }

    cv::flip(scaledImage, scaledImage, 0);

    currentColumn = cv::Mat::zeros(lines, 1, image.type());

    // begin audio stream
    PaError error;
    PaStream* audioStream;
    
    error = Pa_Initialize();
    if (error != paNoError) {
        std::cerr << "ERROR: failed to initialized PortAudio!" << std::endl;
        return false;
    }

    error = Pa_OpenDefaultStream(&audioStream,
                                 0, 2, paFloat32,
                                 AUDIO_SAMPLE_RATE,
                                 256,
                                 ImagePlayer::audioCallback,
                                 this);

    if (error != paNoError) {
        std::cerr << "ERROR: failed to open audio stream!" << std::endl;
        return false;
    }

    error = Pa_StartStream(audioStream);
    if (error != paNoError) {
        std::cerr << "ERROR: failed to start audio stream!" << std::endl;
        return false;
    }

    // play audio for each column
    std::chrono::duration<double> timePerColumn(columnTime);

    for (int i=0; i<scaledImage.cols; i++) {
        columnMutex.lock();
        currentColumn = scaledImage.col(i);
        columnMutex.unlock();

        cv::Mat frame = scaledImage.clone();

        cv::line(frame,
                 cv::Point(i, 0),
                 cv::Point(i, lines-1),
                 128);

        cv::resize(frame, frame, cv::Size(), 4, 4, cv::INTER_NEAREST);
        cv::flip(frame, frame, 0);
        
        cv::imshow("image", frame);
        cv::waitKey(1);

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - begin < timePerColumn) {}
    }

    // close up shop
    error = Pa_StopStream(audioStream);
    if (error != paNoError) {
        std::cerr << "ERROR: failed to stop audio stream!" << std::endl;
        return false;
    }

    error = Pa_CloseStream(audioStream);
    if (error != paNoError) {
        std::cerr << "ERROR: failed to close audio stream!" << std::endl;
        return false;
    }

    Pa_Terminate();

    return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static int ImagePlayer::audioCallback(const void* inputBuffer,
                                      void* outputBuffer,
                                      unsigned long frames,
                                      const PaStreamCallbackTimeInfo* timeInfo,
                                      PaStreamCallbackFlags flags,
                                      void* data)
{
    float* out = (float*) outputBuffer;
    ImagePlayer* player = (ImagePlayer*) data;

    for (int i=0; i<frames; i++) {
        float wave = player->getWaveForm();
        *out++ = wave; *out++ = wave;
    }

    return 0;
}
        
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

float ImagePlayer::getWaveForm()
{
    double phase = t/AUDIO_SAMPLE_RATE;
    t++;
    if (t > AUDIO_SAMPLE_RATE)
        t = 0;

    float output = 0;
    columnMutex.lock();
    for (int i=0; i<lines; i++) {
	double y = ((double) i) / lines;
	double yprime = log(y+1) / log(2);
        double frequency = minFreq + yprime*(maxFreq-minFreq);
	double intensity = 1 - ((double) currentColumn.at<uchar>(i,0))/255;
	if (intensity < 0.25)
	    intensity = 0;
        output += volume * intensity * sin(2*PI*frequency*phase);
    }
    columnMutex.unlock();

    return output;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
