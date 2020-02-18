#ifndef SENSORY_SUBSTITUTION_CALLBACKS_HPP
#define SENSORY_SUBSTITUTION_CALLBACKS_HPP

#include <vector>
#include <mutex>
#include <string>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include "smmServer.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct thresholdSettings {
  int hueMax;   
  int satMax;   
  int valMax;   
  int hueMin;   
  int satMin;   
  int valMin;   
  int erosions; 
  int dilations;
  int gridPercentage;
};

struct glob {
  // camera
  cv::VideoCapture camera;

  // matrices
  cv::Mat frame;
  cv::Mat ballMask;
  cv::Mat bgMask;

  // output vectors
  std::vector<cv::Rect> gridSquares;
  std::vector<bool> ballGrid;
  std::vector<bool> handGrid;
  
  // settings
  std::string settingsFile;
  double imageScaling;
  int gridWidth;
  int gridHeight;
  struct thresholdSettings ball;
  struct thresholdSettings bg;
  
  // mutexes
  std::mutex cameraMutex;
  std::mutex frameMutex;
  std::mutex ballMaskMutex;
  std::mutex bgMaskMutex;
  std::mutex settingsMutex;
  std::mutex gridMutex;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveCameraImage(httpMessage message, void* data);
void serveBallMask(httpMessage message, void* data);
void serveBgMask(httpMessage message, void* data);

void serveBallSettings(httpMessage message, void* data);
void serveBgSettings(httpMessage message, void* data);

void setBallSettings(httpMessage message, void* data);
void setBgSettings(httpMessage message, void* data);
void saveSettings(httpMessage message, void* data);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#endif

/*                        ,d88b.d88b,
                          88888888888
                          `Y8888888Y'
                            `Y888Y'
                              `Y'                                */
