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

struct Thresholders {
    Thresholder* ball;
    Thresholder* hand;
};

void serveSettings(httpMessage msg, void* data, std::string type);
void setSettings(httpMessage msg, void* data);
void saveSettings(httpMessage msg, void* data);
cv::Point findCentroid(cv::Mat binaryImage);

AudioIndicator* ball;
AudioIndicator* hand;

void drawVectorPoints(cv::Mat image, std::vector<cv::Point> points, cv::Scalar color, bool with_numbers) {
    for (int i = 0; i < points.size(); i++) {
	cv::circle(image, points[i], 5, color, 2, 8);
	if(with_numbers)
	    cv::putText(image, std::to_string(i), points[i], cv::FONT_HERSHEY_PLAIN, 3, color);
    }
}

cv::Point average(std::vector<cv::Point> points) {
    double x = 0;
    double y = 0;
    for (size_t k=0; k<points.size(); k++) {
	x += points[k].x;
	y += points[k].y;
    }

    return cv::Point(x/points.size(), y/points.size());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int audioCallback(const void* inputBuffer,
		  void* outputBuffer,
		  unsigned long frames,
		  const PaStreamCallbackTimeInfo* timeInfo,
		  PaStreamCallbackFlags flags,
		  void* data) {
    float* out = (float*) outputBuffer;

    for (int i=0; i<frames; i++) {
	float l = ball->waveLevel();
	float r = hand->waveLevel();
	*out++ = l; *out++ = r;
    }

    return 0;
}
    
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char** argv) {

    ball = new AudioIndicator(100, 500, 3, 40);
    hand = new AudioIndicator(100, 500, 3, 40);

    PaError error;
    PaStream* audioStream;
    
    error = Pa_Initialize();
    if (error != paNoError) {
	std::cerr << "[SS] ERROR: failed to initialize PortAudio!" << std::endl;
	return 1;
    }

    error = Pa_OpenDefaultStream(&audioStream,
				 0, 2, paFloat32,
				 SAMPLE_RATE,
				 256,
				 audioCallback,
				 NULL);
    if (error != paNoError) {
	std::cerr << "[SS] ERROR: failed to open audio stream!" << std::endl;
	return 1;
    }

    error = Pa_StartStream(audioStream);
    if (error != paNoError) {
	std::cerr << "[SS] ERROR: failed to start audio stream!" << std::endl;
	return 1;
    }

    
    // open camera
    cv::VideoCapture camera(1);
    if (!camera.isOpened()) {
        std::cerr << "[SS] FATAL: could not open camera!" << std::endl;
        return 1;
    }

    // load threshold settings
    struct Thresholders th;
    th.ball = new Thresholder("settings.yaml", "ball");
    th.hand = new Thresholder("settings.yaml", "hand");

    // configure server
    smmServer server("8000", "./web_root", &th);
    server.addGetCallback("ballSettings",
			  [](httpMessage msg, void* data)
			  { serveSettings(msg, data, "ball"); });
    server.addGetCallback("handSettings",
			  [](httpMessage msg, void* data)
			  { serveSettings(msg, data, "hand"); });
    server.addPostCallback("settings", &setSettings);
    server.addPostCallback("save", &saveSettings);

    server.launch();

    AudioIndicator indicator(100, 500, 3, 40);

    cv::Mat frame, ballMask, handMask;

    cv::Point handCenter(0,0);
    
    while(true) { //server.isRunning()) {
        camera >> frame;
        ballMask = th.ball->thresholdImage(frame);
	handMask = th.hand->thresholdImage(frame);

	cv::bitwise_xor(ballMask, handMask, handMask);

	std::vector<cv::Vec4i> hierarchy;
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(handMask, contours, hierarchy,
			 cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
	int biggestContourIndex = -1;
	double biggestArea = 1000;

	for (size_t k=0; k<contours.size(); k++) {
	    double area = cv::contourArea(contours[k], false);
	    if (area > biggestArea) {
		biggestContourIndex  = k;
		biggestArea = area;
	    }
	}

	if (biggestContourIndex != -1) {
	    std::vector<int> hullPoints;
	    std::vector<cv::Vec4i> defects;
	    std::vector<cv::Point> defectPoints;
	    cv::convexHull(cv::Mat(contours[biggestContourIndex]), hullPoints, false);
	    if (hullPoints.size() > 3) {
		cv::convexityDefects(contours[biggestContourIndex], hullPoints, defects);
		for (size_t k=0; k<defects.size(); k++)
		    defectPoints.push_back(contours[biggestContourIndex][defects[k].val[0]]);
	    }
	    cv::drawContours(frame, contours, biggestContourIndex, cv::Scalar(0,0,255));
	    drawVectorPoints(frame, defectPoints, cv::Scalar(0,255,255), false);
	    handCenter = 0.6*handCenter + 0.4*average(defectPoints);
	    cv::circle(frame, handCenter, 5, cv::Scalar(255,128,0), 2, 8);
	    hand->setVolume(0.1);
	    hand->update(((double)handCenter.x)/frame.cols, ((double)handCenter.y)/frame.rows);
	    std::cout << handCenter << std::endl;
	}
	else
	    hand->setVolume(0);

        cv::Point centroid = findCentroid(ballMask);
        if (centroid != cv::Point(-1,-1)) {
            float x = ((float)centroid.x) / frame.cols;
            float y = ((float)centroid.y) / frame.rows;

            ball->setVolume(0.1);
            ball->update(x, y);

            cv::circle(frame, centroid, 4, cv::Scalar(0, 0, 255), 1, cv::FILLED);
        }
        else {
            ball->setVolume(0);
        }

        cv::imshow("frame", frame);
        cv::imshow("ball", ballMask);
	cv::imshow("hand", handMask);

	cv::waitKey(10);
    }

    error = Pa_StopStream(audioStream);
    if (error != paNoError) {
	std::cerr << "[INDICATOR] ERROR: failed to stop audio stream!" << std::endl;
	return 1;
    }

    error = Pa_CloseStream(audioStream);
    if (error != paNoError) {
	std::cerr << "[INDICATOR] ERROR: failed to close audio stream!" << std::endl;
	return 1;
    }

    Pa_Terminate();

    delete th.ball;
    delete th.hand;

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

void serveSettings(httpMessage msg, void* data, std::string type) {
    struct Thresholders* ths = (struct Thresholders*) data;
    Thresholder* th = NULL;
    if (type == "ball")
	th = ths->ball;
    else if (type == "hand")
	th = ths->hand;
    else {
	std::cerr << "[SS] WARNING: cannot serve settings for unknown type '"
		  << type
		  << "'; ignoring..." << std::endl;
	msg.replyHttpError(422, "bad type");
	return;
    }
    
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
    struct Thresholders* ths = (struct Thresholders*) data;

    std::string type = msg.getHttpVariable("type");
    Thresholder* th = NULL;
    if (type == "ball")
	th = ths->ball;
    else if (type == "hand")
	th = ths->hand;
    else {
	std::cerr << "[SS] WARNING: cannot configure settings for unknown type '"
		  << type
		  << "'; ignoring..." << std::endl;
	msg.replyHttpError(422, "bad type");
	return;
    }
    
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
    cv::FileStorage fs;
    fs.open("settings.yaml", cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
	std::cerr << "[THRESHOLDER] ERROR: failed to open settings file '"
		  << "settings.yaml" << "'; could not save settings!"
		  << std::endl;
	msg.replyHttpError(500, "file write error!");
	return;
    }
    
    struct Thresholders* ths = (struct Thresholders*) data;

    ths->ball->saveSettings(fs, "ball");
    ths->hand->saveSettings(fs, "hand");

    fs.release();

    std::cout << "saved!" << std::endl;
    
    msg.replyHttpOk();
}



    
