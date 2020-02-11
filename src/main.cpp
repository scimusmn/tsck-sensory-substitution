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

struct glob {
  std::mutex access;
  cv::VideoCapture camera;
  double imageScaling;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void sendFrame(cv::Mat& frame, httpMessage& m) {
  // get raw JPEG bytes from frame
  std::vector<unsigned char> rawJpegBuffer;
  cv::imencode(".jpeg", frame, rawJpegBuffer);

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
  cv::Mat frame;
  g->camera >> frame;
  g->access.unlock();

  cv::resize(frame, frame, cv::Size(), g->imageScaling, g->imageScaling);
  sendFrame(frame, message);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char** argv) {
  struct glob g;
  g.imageScaling = 0.4;
  g.camera = cv::VideoCapture(1);
  if (!g.camera.isOpened()) {
    std::cerr << "FATAL: could not open camera!" << std::endl;
    return 1;
  }

  std::string httpPort = "8000";
  std::string rootPath = "./web_root";

  smmServer server(httpPort, rootPath, &g);
  server.addGetCallback ("cameraImage",  &serveCameraImage );
  server.launch();

  while(server.isRunning()) {}

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
