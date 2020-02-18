#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include "smmServer.hpp"
#include "callbacks.hpp"
#include "util.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char** argv) {

  // important variables
  struct glob g;
  g.settingsFile = "settings.yaml"; // mask settings

  if (!loadSettings(&g)) {
    std::cerr << "FATAL: could not load settings file '" << g.settingsFile << "'; aborting!" << std::endl;
    return 2;
  }
  std::cout << "loaded settings." << std::endl;

  // open camera
  g.camera = cv::VideoCapture(0);
  if (!g.camera.isOpened()) {
    std::cerr << "FATAL: could not open camera!" << std::endl;
    return 1;
  }
  std::cout << "opened camera." << std::endl;

  if (setupGrid(&g) != 0) {
    std::cerr << "FATAL: could not configure PCA9685; aborting!" << std::endl;
    return 3;
  }

  smmServer server("8000", "./web_root", &g);

  // image GET callbacks
  server.addGetCallback("cameraImage",  &serveCameraImage );
  server.addGetCallback("ballMask", &serveBallMask);
  server.addGetCallback("bgMask", &serveBgMask);

  // settings GET callbacks
  server.addGetCallback("ballSettings", &serveBallSettings);
  server.addGetCallback("bgSettings", &serveBgSettings);

  // settings POST callbacks
  server.addPostCallback("setBallSettings", &setBallSettings);
  server.addPostCallback("setBgSettings", &setBgSettings);
  server.addPostCallback("saveSettings", &saveSettings);

  std::cout << "loaded callbacks." << std::endl;

  // begin server
  server.launch();

  std::cout << "Server started on port 8000" << std::endl;
  
  while(server.isRunning()) {
    g.frameMutex.lock();
    g.camera >> g.frame;

    g.settingsMutex.lock();
    g.ballMaskMutex.lock();
    g.ballMask = getMask(g.frame, g.ball);
    g.ballMaskMutex.unlock();

    g.bgMaskMutex.lock();
    g.bgMask = getMask(g.frame, g.bg);
    g.bgMaskMutex.unlock();
    g.settingsMutex.unlock();
    g.frameMutex.unlock();

    updateGrid(&g);

    cv::waitKey(10);
  }

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*                        ,d88b.d88b,
                          88888888888
                          `Y8888888Y'
                            `Y888Y'
                              `Y'                                */
