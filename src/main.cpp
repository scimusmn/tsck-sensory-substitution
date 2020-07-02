#include <iostream>
#include <vector>
#include <mutex>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include "smmServer.hpp"
#include "thresholder.hpp"
#include "audioIndicator.hpp"

extern "C" {
#include "b64/base64.h"
}

void serveSettings(httpMessage msg, void* data);
void setSettings(httpMessage msg, void* data);
void saveSettings(httpMessage msg, void* data);
cv::Point findCentroid(cv::Mat binaryImage);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char** argv) {
    // open camera
    cv::VideoCapture camera(0);
    if (!camera.isOpened()) {
        std::cerr << "FATAL: could not open camera!" << std::endl;
        return 1;
    }

    // load threshold settings
    Thresholder th("settings.yaml", "ball");
    th.printValues();

    // configure server
    smmServer server("8000", "./web_root", &th);
    server.addGetCallback("settings", &serveSettings);
    server.addPostCallback("settings", &setSettings);
    server.addPostCallback("save", &saveSettings);

    server.launch();

    AudioIndicator indicator(100, 500, 3, 40);
        
    cv::Mat frame, thresh;
    while(server.isRunning()) {
        camera >> frame;
        thresh = th.thresholdImage(frame);

        cv::Point centroid = findCentroid(thresh);
        if (centroid != cv::Point(-1,-1)) {
            float x = ((float)centroid.x) / frame.cols;
            float y = ((float)centroid.y) / frame.rows;

            indicator.setVolume(0.1);
            indicator.update(x, y);

            cv::circle(frame, centroid, 4, cv::Scalar(0, 0, 255), 1, cv::FILLED);
        }
        else {
            indicator.setVolume(0);
        }

        cv::imshow("frame", frame);
        cv::imshow("thresh", thresh);

        cv::waitKey(10);
    }

    return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cv::Point findCentroid(cv::Mat binaryImage) {
    std::vector<cv::Point> points;
    cv::findNonZero(binaryImage, points);

    cv::Point_<float> centroid(0,0);
    float a = 1.0f/points.size();
    for (auto p=points.begin(); p != points.end(); p++) {
        centroid.x += a * p->x;
        centroid.y += a * p->y;
    }

    cv::Point result;
    if (centroid.x == 0.0f && centroid.y == 0.0f) {
        result = cv::Point(-1,-1);
    }
    else {
        result.x = centroid.x;
        result.y = centroid.y;
    }

    return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void serveSettings(httpMessage msg, void* data) {
    Thresholder* th = (Thresholder*) data;

    std::string buffer = "{";
    buffer += "\"hueMin\":";
    buffer += std::to_string(th->hMin);
    buffer += ",\"satMin\":";
    buffer += std::to_string(th->sMin);
    buffer += ",\"valMin\":";
    buffer += std::to_string(th->vMin);

    buffer += ",\"hueMax\":";
    buffer += std::to_string(th->hMax);
    buffer += ",\"satMax\":";
    buffer += std::to_string(th->sMax);
    buffer += ",\"valMax\":";
    buffer += std::to_string(th->vMax);

    buffer += ",\"erosions\":";
    buffer += std::to_string(th->erosions);
    buffer += ",\"dilations\":";
    buffer += std::to_string(th->dilations);
    buffer += "}";

    msg.replyHttpContent("text/plain", buffer);
}

void setSettings(httpMessage msg, void* data) {
    Thresholder* th = (Thresholder*) data;

    int hMin = std::stoi(msg.getHttpVariable("hueMin"));
    int sMin = std::stoi(msg.getHttpVariable("satMin"));
    int vMin = std::stoi(msg.getHttpVariable("valMin"));

    int hMax = std::stoi(msg.getHttpVariable("hueMax"));
    int sMax = std::stoi(msg.getHttpVariable("satMax"));
    int vMax = std::stoi(msg.getHttpVariable("valMax"));

    int erosions  = std::stoi(msg.getHttpVariable("erosions"));
    int dilations = std::stoi(msg.getHttpVariable("dilations"));

    th->setValues(hMin, hMax,
                  sMin, sMax,
                  vMin, vMax,
                  erosions, dilations);

    msg.replyHttpOk();
}

void saveSettings(httpMessage msg, void* data) {
    Thresholder* th = (Thresholder*) data;

    th->saveSettings("settings.yaml", "ball");
    msg.replyHttpOk();
}



    
