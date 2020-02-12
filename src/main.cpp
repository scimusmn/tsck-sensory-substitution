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

void serveBallSettings(httpMessage message, void* data);
void setBallSettings(httpMessage message, void* data);

void serveBgSettings(httpMessage message, void* data);
void setBgSettings(httpMessage message, void* data);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char** argv) {
  struct glob g;
  g.imageScaling = 0.25;
  g.ball = { 179, 255, 255, 0, 0, 0, 0, 0 };
  g.bg   = { 179, 255, 255, 0, 0, 0, 0, 0 };
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

  server.addGetCallback("ballSettings", &serveBallSettings);
  server.addPostCallback("setBallSettings", &setBallSettings);

  server.addGetCallback("bgSettings", &serveBgSettings);
  server.addPostCallback("setBgSettings", &setBgSettings);
  
  server.launch();

  std::cout << "Server started on port " << httpPort << std::endl;
  
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
  if (s.hueMin < s.hueMax) {
    cv::threshold(chan[0],mask,s.hueMax, 255, cv::THRESH_BINARY_INV);
    cv::threshold(chan[0],mask_tmp,s.hueMin-1, 255, cv::THRESH_BINARY);
    cv::bitwise_and(mask_tmp,mask,mask);
  }
  else {
    cv::threshold(chan[0],mask,s.hueMin-1, 255, cv::THRESH_BINARY);
    cv::threshold(chan[0],mask_tmp,s.hueMax, 255, cv::THRESH_BINARY_INV);
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveBallMask(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  cv::Mat mask;
  bool ok = false;
  
  g->access.lock();
  if (!g->frame.empty()) {
    mask = getMask(g->frame, g->ball);
    ok = true;
  }
  g->access.unlock();

  if (ok) {
    sendMat(mask, message);
  }
  else {
    message.replyHttpError(503, "Frame not yet loaded");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveBgMask(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  cv::Mat mask;
  bool ok = false;
  
  g->access.lock();
  if (!g->frame.empty()) {
    mask = getMask(g->frame, g->bg);
    ok = true;
  }
  g->access.unlock();

  if (ok) {
    sendMat(mask, message);
  }
  else {
    message.replyHttpError(503, "Frame not yet loaded");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveBallSettings(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  g->access.lock();
  
  std::string buffer = "{";
  buffer += "\"hueMax\":";
  buffer += std::to_string(g->ball.hueMax);
  buffer += ",\"hueMin\":";   
  buffer += std::to_string(g->ball.hueMin);
  buffer += ",\"satMax\":";   
  buffer += std::to_string(g->ball.satMax);
  buffer += ",\"satMin\":";   
  buffer += std::to_string(g->ball.satMin);
  buffer += ",\"valMax\":";   
  buffer += std::to_string(g->ball.valMax);
  buffer += ",\"valMin\":";   
  buffer += std::to_string(g->ball.valMin);
  buffer += ",\"erosions\":";
  buffer += std::to_string(g->ball.erosions);
  buffer += ",\"dilations\":";
  buffer += std::to_string(g->ball.dilations);
  buffer += "}";
  
  message.replyHttpContent("text/plain", buffer);
  g->access.unlock();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
void setBallSettings(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  struct thresholdSettings settings;
  settings.hueMax    = std::stoi(message.getHttpVariable("hueMax"));
  settings.satMax    = std::stoi(message.getHttpVariable("satMax"));
  settings.valMax    = std::stoi(message.getHttpVariable("valMax"));
  settings.hueMin    = std::stoi(message.getHttpVariable("hueMin"));   
  settings.satMin    = std::stoi(message.getHttpVariable("satMin"));   
  settings.valMin    = std::stoi(message.getHttpVariable("valMin"));   
  settings.erosions  = std::stoi(message.getHttpVariable("erosions"));   
  settings.dilations = std::stoi(message.getHttpVariable("dilations"));
  
  g->access.lock();
  g->ball = settings;
  g->access.unlock();

  message.replyHttpOk();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveBgSettings(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  g->access.lock();
  
  std::string buffer = "{";
  buffer += "\"hueMax\":";
  buffer += std::to_string(g->bg.hueMax);
  buffer += ",\"hueMin\":";   
  buffer += std::to_string(g->bg.hueMin);
  buffer += ",\"satMax\":";   
  buffer += std::to_string(g->bg.satMax);
  buffer += ",\"satMin\":";   
  buffer += std::to_string(g->bg.satMin);
  buffer += ",\"valMax\":";   
  buffer += std::to_string(g->bg.valMax);
  buffer += ",\"valMin\":";   
  buffer += std::to_string(g->bg.valMin);
  buffer += ",\"erosions\":";
  buffer += std::to_string(g->bg.erosions);
  buffer += ",\"dilations\":";
  buffer += std::to_string(g->bg.dilations);
  buffer += "}";
  
  message.replyHttpContent("text/plain", buffer);
  g->access.unlock();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setBgSettings(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  struct thresholdSettings settings;
  settings.hueMax    = std::stoi(message.getHttpVariable("hueMax"));
  settings.satMax    = std::stoi(message.getHttpVariable("satMax"));
  settings.valMax    = std::stoi(message.getHttpVariable("valMax"));
  settings.hueMin    = std::stoi(message.getHttpVariable("hueMin"));   
  settings.satMin    = std::stoi(message.getHttpVariable("satMin"));   
  settings.valMin    = std::stoi(message.getHttpVariable("valMin"));   
  settings.erosions  = std::stoi(message.getHttpVariable("erosions"));   
  settings.dilations = std::stoi(message.getHttpVariable("dilations"));
  
  g->access.lock();
  g->bg = settings;
  g->access.unlock();

  message.replyHttpOk();
}  

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
