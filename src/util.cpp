#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vector>

extern "C" {
  #include "b64/base64.h"
}

#include "util.hpp"

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

void setupGrid(struct glob* g) {
  int nGridSquares = g->gridWidth * g->gridHeight;

  g->settingsMutex.lock();
  g->cameraMutex.lock();
  g->frameMutex.lock();
  g->camera >> g->frame;
  int squareWidth = g->frame.cols/g->gridWidth;
  int squareHeight = g->frame.rows/g->gridHeight;
  g->frameMutex.unlock();
  g->cameraMutex.unlock();
  g->settingsMutex.unlock();

  g->gridMutex.lock();
  for (int x=0; x<g->gridWidth; x++) {
    for (int y=0; y<g->gridHeight; y++) {
      g->gridSquares.push_back(cv::Rect(x*squareWidth, y*squareHeight, squareWidth, squareHeight));
    }
  }
  g->gridMutex.unlock();
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

  std::cout << "----------------" << std::endl;

  g->gridMutex.lock();
  for (int i=0; i<g->gridSquares.size(); i++) {
    double handRatio = double(cv::countNonZero(handMask(g->gridSquares[i]))) / g->gridSquares[i].area();
    double ballRatio = double(cv::countNonZero(g->ballMask(g->gridSquares[i]))) / g->gridSquares[i].area();

    std::cout << "(" << handRatio << ", " << ballRatio << ")" << std::endl;
  }
  g->gridMutex.unlock();
  g->ballMaskMutex.unlock();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
