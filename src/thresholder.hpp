#ifndef SMM_THRESHOLDER_H
#define SMM_THRESHOLDER_H

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

class Thresholder {
private:
    void setDefaultValues() {
        hMin = 0;
        sMin = 0;
        vMin = 0;
        hMax = 255;
        sMax = 255;
        vMax = 255;
        erosions = 0;
        dilations = 0;
    }

public:
    int hMin, sMin, vMin;
    int hMax, sMax, vMax;
    int erosions, dilations;
    
    Thresholder(std::string settingsFilename, std::string thresholderName) {
        cv::FileStorage fs;
        fs.open(settingsFilename.c_str(), cv::FileStorage::READ);
        if (!fs.isOpened()) {
            std::cerr << "[THRESHOLDER] ERROR: failed to open settings file '"
                      << settingsFilename << "'; using default values instead."
                      << std::endl;
            setDefaultValues();
            return;
        }

        cv::FileNode node = fs[thresholderName.c_str()];
        if (node.empty()) {
            std::cerr << "[THRESHOLDER] ERROR: failed to find node '"
                      << thresholderName << "'; using default values instead."
                      << std::endl;
            setDefaultValues();
            return;
        }
        
        node["hue_minimum"]        >> hMin;   
        node["saturation_minimum"] >> sMin;   
        node["value_minimum"]      >> vMin;   

        node["hue_maximum"]        >> hMax;    
        node["saturation_maximum"] >> sMax;   
        node["value_maximum"]      >> vMax;   

        node["erosions"]           >> erosions; 
        node["dilations"]          >> dilations;

        fs.release();
    }

    void saveSettings(cv::FileStorage& fs, std::string thresholderName) {
        fs << thresholderName << "{";
        fs << "hue_minimum"        << hMin;  
        fs << "saturation_minimum" << sMin;  
        fs << "value_minimum"      << vMin;  

        fs << "hue_maximum"        << hMax;  
        fs << "saturation_maximum" << sMax;  
        fs << "value_maximum"      << vMax;  

        fs << "erosions"  << erosions;
        fs << "dilations" << dilations;
        fs << "}";
    }

    void printValues() {
        std::cout << "hue: (" << hMin << "-" << hMax << ")" << std::endl;
        std::cout << "saturation: (" << sMin << "-" << sMax << ")" << std::endl;
        std::cout << "value: (" << vMin << "-" << vMax << ")" << std::endl;
        std::cout << "erosions: " << erosions << std::endl;
        std::cout << "dilations: " << dilations << std::endl;
    }

    cv::Mat thresholdImage(cv::Mat mat) {
        std::vector<cv::Mat> channels;
        cv::Mat hsv, thresh, thresh_tmp;

        cv::cvtColor(mat, hsv, cv::COLOR_BGR2HSV);
        cv::split(hsv, channels);

        // hue
        cv::threshold(channels[0], thresh_tmp, hMax,   255, cv::THRESH_BINARY_INV);
        cv::threshold(channels[0], thresh,     hMin-1, 255, cv::THRESH_BINARY);
        cv::bitwise_and(thresh_tmp, thresh, thresh);

        // saturation
        cv::threshold(channels[1], thresh_tmp, sMax,   255, cv::THRESH_BINARY_INV);
        cv::bitwise_and(thresh_tmp, thresh, thresh);
        cv::threshold(channels[1], thresh_tmp, sMin-1, 255, cv::THRESH_BINARY);
        cv::bitwise_and(thresh_tmp, thresh, thresh);

        // value
        cv::threshold(channels[2], thresh_tmp, vMax,   255, cv::THRESH_BINARY_INV);
        cv::bitwise_and(thresh_tmp, thresh, thresh);
        cv::threshold(channels[2], thresh_tmp, vMin-1, 255, cv::THRESH_BINARY);
        cv::bitwise_and(thresh_tmp, thresh, thresh);

        // erode/dilate
        cv::erode (thresh, thresh, cv::Mat(), cv::Point(-1, -1), erosions);
        cv::dilate(thresh, thresh, cv::Mat(), cv::Point(-1, -1), dilations);

        return thresh;
    }

    void setValues(int hueMin, int hueMax,
                   int satMin, int satMax,
                   int valMin, int valMax,
                   int numErosions, int numDilations) {
        hMin = hueMin;
        sMin = satMin;
        vMin = valMin;
        
        hMax = hueMax;
        sMax = satMax;
        vMax = valMax;

        erosions = numErosions;
        dilations = numDilations;
    }
};
    
#endif
