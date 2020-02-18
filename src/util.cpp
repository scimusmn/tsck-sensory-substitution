#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vector>
#include <cmath>

extern "C" {
  #include <wiringPi.h>
  #include <wiringPiI2C.h>
  #include "pca9685/pca9685.h"
  #include "b64/base64.h"
}

#include "util.hpp"

// PCA9685 constants
static const int PIN_BASE=300;
static const int MAX_PWM=4096;
static const int HERTZ=50;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void sendMat(cv::Mat& mat, httpMessage& m) {
  // get raw JPEG bytes from frame
  std::vector<unsigned char> rawJpegBuffer;
  cv::imencode(".jpeg", mat, rawJpegBuffer);

  // convert to base64
  unsigned int bufferSize = b64e_size(rawJpegBuffer.size())+1;
  unsigned char* b64JpegBuffer = (unsigned char*) malloc(bufferSize * sizeof(unsigned char));
  b64_encode(rawJpegBuffer.data(), rawJpegBuffer.size(), b64JpegBuffer);
  
  m.replyHttpContent("image/jpeg", std::string((char*) b64JpegBuffer, bufferSize));
  free(b64JpegBuffer);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cv::Mat getMask(cv::Mat& frame, struct thresholdSettings s) {
  std::vector<cv::Mat> chan;
  cv::Mat hsv, mask_tmp, mask;

  // build mask
  cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
  cv::split(hsv,chan);
  if (s.hueMin < s.hueMax) {
    cv::threshold(chan[0],mask,s.hueMax, 255, cv::THRESH_BINARY_INV);
    cv::threshold(chan[0],mask_tmp,s.hueMin-1, 255, cv::THRESH_BINARY);
    cv::bitwise_and(mask_tmp,mask,mask);
  }
  else {
    cv::threshold(chan[0],mask,s.hueMax, 255, cv::THRESH_BINARY_INV);
    cv::threshold(chan[0],mask_tmp,s.hueMin-1, 255, cv::THRESH_BINARY);
    cv::bitwise_or(mask_tmp,mask,mask);
  }

  cv::threshold(chan[1],mask_tmp,s.satMax, 255, cv::THRESH_BINARY_INV);
  cv::bitwise_and(mask_tmp,mask,mask);
  cv::threshold(chan[1],mask_tmp,s.satMin-1, 255, cv::THRESH_BINARY);
  cv::bitwise_and(mask_tmp,mask,mask);

  cv::threshold(chan[2],mask_tmp,s.valMax, 255, cv::THRESH_BINARY_INV);
  cv::bitwise_and(mask_tmp,mask,mask);    
  cv::threshold(chan[2],mask_tmp,s.valMin-1, 255, cv::THRESH_BINARY);
  cv::bitwise_and(mask_tmp,mask,mask);

  // erode / dilate mask
  cv::erode(mask,mask,cv::Mat(),cv::Point(-1,-1),s.erosions);
  cv::dilate(mask,mask,cv::Mat(),cv::Point(-1,-1),s.dilations);

  return mask;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool loadSettings(struct glob* g) {
  cv::FileStorage fs;
  fs.open(g->settingsFile, cv::FileStorage::READ);

  if (!fs.isOpened()) {
    return false;
  }

  g->settingsMutex.lock();

  fs["imageScaling"] >> g->imageScaling;
  fs["gridWidth"]   >> g->gridWidth;
  fs["gridHeight"]  >> g->gridHeight;
  
  cv::FileNode node = fs["ballSettings"];
  node["hueMax"]    >> g->ball.hueMax;    
  node["satMax"]    >> g->ball.satMax;   
  node["valMax"]    >> g->ball.valMax;   
  node["hueMin"]    >> g->ball.hueMin;   
  node["satMin"]    >> g->ball.satMin;   
  node["valMin"]    >> g->ball.valMin;   
  node["erosions"]  >> g->ball.erosions; 
  node["dilations"] >> g->ball.dilations;

  node = fs["bgSettings"];
  node["hueMax"]    >> g->bg.hueMax;    
  node["satMax"]    >> g->bg.satMax;   
  node["valMax"]    >> g->bg.valMax;   
  node["hueMin"]    >> g->bg.hueMin;   
  node["satMin"]    >> g->bg.satMin;   
  node["valMin"]    >> g->bg.valMin;   
  node["erosions"]  >> g->bg.erosions; 
  node["dilations"] >> g->bg.dilations;

  g->settingsMutex.unlock();
  
  return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int setupGrid(struct glob* g) {
  wiringPiSetup();
  int setupResult = pca9685Setup(PIN_BASE, 0x40, HERTZ);
  if (setupResult < 0) {
    std::cerr << "ERROR: pca9685 setup returned " << setupResult << std::endl;
    return 1;
  }

  pca9685PWMReset(setupResult);
  
  int nGridSquares = g->gridWidth * g->gridHeight;

  g->settingsMutex.lock();
  g->cameraMutex.lock();
  g->frameMutex.lock();
  g->camera >> g->frame;
  cv::resize(g->frame, g->frame, cv::Size(), g->imageScaling, g->imageScaling);
  int squareWidth = g->frame.cols/g->gridWidth;
  int squareHeight = g->frame.rows/g->gridHeight;
  g->frameMutex.unlock();
  g->cameraMutex.unlock();
  g->settingsMutex.unlock();

  g->gridMutex.lock();
  for (int y=g->gridHeight-1; y>=0; y--) {
    for (int x=0; x<g->gridWidth; x++) {
      g->gridSquares.push_back(cv::Rect(x*squareWidth, y*squareHeight, squareWidth, squareHeight));
    }
  }
  g->gridMutex.unlock();

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void updateGrid(struct glob* g) {
  cv::Mat handMask;

  g->ballMaskMutex.lock();
  g->bgMaskMutex.lock();

  handMask = cv::Mat::zeros(g->ballMask.rows, g->ballMask.cols, g->ballMask.type());
  cv::bitwise_or(g->ballMask, g->bgMask, handMask);
  cv::bitwise_not(handMask, handMask);

  g->bgMaskMutex.unlock();

  g->gridMutex.lock();
  for (int i=0; i<g->gridSquares.size(); i++) {
    double handRatio = double(cv::countNonZero(handMask(g->gridSquares[i]))) / g->gridSquares[i].area();
    double ballRatio = double(cv::countNonZero(g->ballMask(g->gridSquares[i]))) / g->gridSquares[i].area();

    double total = handRatio + ballRatio;
    if (total > 0) {
      total = total + 0.8;
    }
    if (total > 1) {
      total = 1;
    }
    int pwmLevel = int ( MAX_PWM * total );

    pwmWrite(PIN_BASE + i, pwmLevel);
  }
  g->gridMutex.unlock();
  g->ballMaskMutex.unlock();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
