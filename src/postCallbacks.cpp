#include "callbacks.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setBallSettings(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  struct thresholdSettings settings;
  try {
    settings.hueMax    = std::stoi(message.getHttpVariable("hueMax"));
    settings.satMax    = std::stoi(message.getHttpVariable("satMax"));
    settings.valMax    = std::stoi(message.getHttpVariable("valMax"));
    settings.hueMin    = std::stoi(message.getHttpVariable("hueMin"));   
    settings.satMin    = std::stoi(message.getHttpVariable("satMin"));   
    settings.valMin    = std::stoi(message.getHttpVariable("valMin"));   
    settings.erosions  = std::stoi(message.getHttpVariable("erosions"));   
    settings.dilations = std::stoi(message.getHttpVariable("dilations"));
  }
  catch(std::invalid_argument error) {
    std::cerr << "error: invalid argument encountered in setBallSettings()" << std::endl;
    message.replyHttpError(422,"Invalid number");
    return;
  }
  
  g->settingsMutex.lock();
  g->ball = settings;
  g->settingsMutex.unlock();

  message.replyHttpOk();
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void setBgSettings(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;

  struct thresholdSettings settings;
  try {
    settings.hueMax    = std::stoi(message.getHttpVariable("hueMax"));
    settings.satMax    = std::stoi(message.getHttpVariable("satMax"));
    settings.valMax    = std::stoi(message.getHttpVariable("valMax"));
    settings.hueMin    = std::stoi(message.getHttpVariable("hueMin"));   
    settings.satMin    = std::stoi(message.getHttpVariable("satMin"));   
    settings.valMin    = std::stoi(message.getHttpVariable("valMin"));   
    settings.erosions  = std::stoi(message.getHttpVariable("erosions"));   
    settings.dilations = std::stoi(message.getHttpVariable("dilations"));
  }
  catch (std::invalid_argument error) {
    std::cerr << "error: invalid argument encountered in setBgSettings()" << std::endl;
    message.replyHttpError(422,"Invalid number");
    return;
  }
  
  g->settingsMutex.lock();
  g->bg = settings;
  g->settingsMutex.unlock();

  message.replyHttpOk();
}  

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void saveSettings(httpMessage message, void* data) {
  struct glob* g = (struct glob*) data;
  
  cv::FileStorage fs;
  fs.open(g->settingsFile, cv::FileStorage::WRITE);

  if (!fs.isOpened()) {
    message.replyHttpError(500, "Could not open settings file!");
    return;
  }

  g->settingsMutex.lock();

  fs << "imageScaling" << g->imageScaling;
  fs << "gridWidth"   << g->gridWidth;
  fs << "gridHeight"  << g->gridHeight;

  fs << "ballSettings" << "{";
  fs << "hueMax"    << g->ball.hueMax;  
  fs << "satMax"    << g->ball.satMax;  
  fs << "valMax"    << g->ball.valMax;  
  fs << "hueMin"    << g->ball.hueMin;  
  fs << "satMin"    << g->ball.satMin;  
  fs << "valMin"    << g->ball.valMin;  
  fs << "erosions"  << g->ball.erosions;
  fs << "dilations" << g->ball.dilations;
  fs << "}";

  fs << "bgSettings" << "{";
  fs << "hueMax"    << g->bg.hueMax;  
  fs << "satMax"    << g->bg.satMax;  
  fs << "valMax"    << g->bg.valMax;  
  fs << "hueMin"    << g->bg.hueMin;  
  fs << "satMin"    << g->bg.satMin;  
  fs << "valMin"    << g->bg.valMin;  
  fs << "erosions"  << g->bg.erosions;
  fs << "dilations" << g->bg.dilations;
  fs << "}";

  g->settingsMutex.unlock();

  std::cout << "saved." << std::endl;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
