#include "callbacks.hpp"
#include "util.hpp"

#include <opencv2/imgproc.hpp>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveCameraImage(httpMessage message,
                      void* data) {
  struct glob* g = (struct glob*) data;

  cv::Mat resizedFrame;
  
  g->frameMutex.lock();
  if (g->frame.empty()) {
    message.replyHttpError(503, "Frame not ready");
    g->frameMutex.unlock();
    return;
  }
  cv::resize(g->frame, resizedFrame, cv::Size(), g->imageScaling, g->imageScaling);
  g->frameMutex.unlock();
  
  sendMat(resizedFrame, message);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveBallMask(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  cv::Mat resizedBallMask;
  
  g->ballMaskMutex.lock();
  if (g->ballMask.empty()) {
    message.replyHttpError(503, "Ball mask not ready");
    g->ballMaskMutex.unlock();
    return;
  }
  cv::resize(g->ballMask, resizedBallMask, cv::Size(), g->imageScaling, g->imageScaling);
  g->ballMaskMutex.unlock();
  
  sendMat(resizedBallMask, message);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveBgMask(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  cv::Mat resizedBgMask;
  
  g->bgMaskMutex.lock();
  if (g->bgMask.empty()) {
    message.replyHttpError(503, "Background mask not ready");
    g->bgMaskMutex.unlock();
    return;
  }
  cv::resize(g->bgMask, resizedBgMask, cv::Size(), g->imageScaling, g->imageScaling);
  g->bgMaskMutex.unlock();
  
  sendMat(resizedBgMask, message);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveBallSettings(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  g->settingsMutex.lock();
  
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
  g->settingsMutex.unlock();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveBgSettings(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  g->settingsMutex.lock();
  
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
  g->settingsMutex.unlock();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
