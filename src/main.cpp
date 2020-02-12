#include <iostream>
#include <vector>
#include <mutex>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include "smmServer.hpp"

extern "C" {
  #include "b64/base64.h"
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct thresholdSettings {
  unsigned int hueMax;
  unsigned int satMax;
  unsigned int valMax;
  unsigned int hueMin;
  unsigned int satMin;
  unsigned int valMin;  
  unsigned int erosions;
  unsigned int dilations;
};

struct glob {
  std::mutex access;
  cv::VideoCapture camera;
  cv::Mat frame;
  double imageScaling;
  struct thresholdSettings ball;
  struct thresholdSettings bg;
};


void sendMat(cv::Mat& frame, httpMessage& m);
void serveCameraImage(httpMessage message, void* data);
cv::Mat getMask(cv::Mat& frame, struct thresholdSettings s);
void serveBallMask(httpMessage message, void* data);
void serveBgMask(httpMessage message, void* data);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char** argv) {
  struct glob g;
  g.imageScaling = 0.4;
  g.ball = { 179, 255, 255, 30, 0, 0, 0, 0 };
  g.bg   = { 30, 255, 255, 0, 0, 0, 0, 0 };
  g.camera = cv::VideoCapture(1);
  if (!g.camera.isOpened()) {
    std::cerr << "FATAL: could not open camera!" << std::endl;
    return 1;
  }

  std::string httpPort = "8000";
  std::string rootPath = "./web_root";

  smmServer server(httpPort, rootPath, &g);
  server.addGetCallback("cameraImage",  &serveCameraImage );
  server.addGetCallback("ballMask", &serveBallMask);
  server.addGetCallback("bgMask", &serveBgMask);
  server.launch();

  while(server.isRunning()) {}

  return 0;
}

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
  

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveCameraImage(httpMessage message,
                      void* data) {
  struct glob* g = (struct glob*) data;

  g->access.lock();
  g->camera >> g->frame;
  cv::resize(g->frame, g->frame, cv::Size(), g->imageScaling, g->imageScaling);
  sendMat(g->frame, message);
  g->access.unlock();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cv::Mat getMask(cv::Mat& frame, struct thresholdSettings s) {
  std::vector<cv::Mat> chan;
  cv::Mat hsv, mask_tmp, mask;

  // build mask
  cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
  cv::split(hsv,chan);
  cv::threshold(chan[0],mask,s.hueMax, 255, cv::THRESH_BINARY_INV);
  cv::threshold(chan[0],mask_tmp,s.hueMin-1, 255, cv::THRESH_BINARY);
  cv::bitwise_and(mask_tmp,mask,mask);

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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveBallMask(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  cv::Mat mask;
  
  g->access.lock();
  mask = getMask(g->frame, g->ball);
  g->access.unlock();
  
  sendMat(mask, message);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveBgMask(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  cv::Mat mask;
  
  g->access.lock();
  mask = getMask(g->frame, g->bg);
  g->access.unlock();

  sendMat(mask, message);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
