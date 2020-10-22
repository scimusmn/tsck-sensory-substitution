#include <iostream>
#include <vector>
#include <mutex>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>

#include "ImagePlayer.hpp"

extern "C" {
#include <unistd.h>
#include "b64/base64.h"
}

#define FREQ_MIN 200.0
#define FREQ_MAX 1000.0
#define VOLUME 0.5
#define LINES 80
#define COLUMN_TIME 0.05

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void printHelp()
{
    std::cout << "Usage: substitution [OPTIONS] [image file]" << std::endl
	      << "  -h        Show this help message and exit." << std::endl
	      << "  -f FILE   Calibration file to use for un-distortion. Omitting this option" << std::endl
	      << "            disables un-distortion." << std::endl
	      << "  -c ID     Camera to use. This option causes the program to ignore any image" << std::endl
	      << "            file that may have been passed."
	      << std::endl;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool parseArgs(int argc, char** argv,
	       int& camera,
	       std::string& calibrationFile,
	       std::string& imageFile)
{
    camera = -1;
    calibrationFile = "";

    opterr = 0;
    int c;

    while( (c = getopt(argc, argv, "hc:f:")) != -1) {
	switch (c) {
	case 'h':
	    return false;

	case 'c':
	    try { camera = std::stoi(optarg); }
	    catch(std::invalid_argument err) {
		std::cerr << "ERROR: '"
			  << optarg
			  << "' is not a valid integer!"
			  << std::endl;
		return false;
	    }
	    break;

	case 'f':
	    calibrationFile = optarg;
	    break;

	case '?':
	    std::cerr << "ERROR: unknown option '"
		      << optopt
		      << "'" << std::endl;
	    return false;

	default:
	    return false;
	}
    }

    if (optind < argc)
	imageFile = argv[optind];
    else
	imageFile = "";

    return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool loadMaps(std::string filename,
	      cv::Mat& map1,
	      cv::Mat& map2)
{
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    if (!fs.isOpened()) {
	std::cerr << "ERROR: failed to open calibration file '"
		  << filename
		  << "'; aborting!" << std::endl;
	return false;
    }

    cv::Mat cameraMatrix, distCoeffs;
    cv::Size imageSize;

    fs["cameraMatrix"] >> cameraMatrix;
    fs["distCoeffs"] >> distCoeffs;
    fs["imageSize"] >> imageSize;

    fs.release();

        cv::initUndistortRectifyMap
        (cameraMatrix, distCoeffs, cv::Mat(),
         cv::getOptimalNewCameraMatrix(cameraMatrix,
                                       distCoeffs,
                                       imageSize, 1,
                                       imageSize, 0),
         imageSize,
         CV_16SC2, map1, map2);

    return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void quadratic(cv::Mat in, cv::Mat& out, double a0, double a1, double a2)
{
    cv::Mat result = cv::Mat::zeros(in.size(), in.type());

    for (int y=0; y<in.rows; y++) {
	for (int x=0; x<in.cols; x++) {
	    unsigned char z = in.at<unsigned char>(y,x);
	    int val = a0 + a1*z + a2*z*z;
	    if (val<0)
		val = 0;
	    if (val>255)
		val = 255;
	    result.at<unsigned char>(y,x) = val;
	}
    }
    result.copyTo(out);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void playCamera(int cameraId, bool undistort,
		cv::Mat map1, cv::Mat map2)
{
    cv::VideoCapture camera(cameraId);
    if (!camera.isOpened()) {
	std::cerr << "ERROR: could not open camera '"
		  << cameraId
		  << "'" << std::endl;
	return;
    }

    ImagePlayer player(FREQ_MIN, FREQ_MAX, VOLUME, LINES, COLUMN_TIME);

    cv::Mat frame, undist, warped, transform, grey, highContrast, ultraContrast;
    std::vector<double> rotationVector = { 0, 0.1, 0 };

    double w = 640;
    double h = 480;
    double f = 600;
    double dist = 0;
    double alpha = 0;

    std::vector<cv::Point2f> initialCorners =
	{ cv::Point2f(0.23*w, 0.50000*h),
	  cv::Point2f(0.84*w, 0.50000*h),
	  cv::Point2f(0.09*w, 0.88000*h),
	  cv::Point2f(0.99*w, 0.88000*h) };

    std::vector<cv::Point2f> finalCorners =
	{ cv::Point2f(0, 0),
	  cv::Point2f(w, 0),
	  cv::Point2f(0, h),
	  cv::Point2f(w, h) };    

    transform = cv::getPerspectiveTransform(initialCorners, finalCorners);
    std::cout << transform << std::endl;
    
    while (true) {
	camera >> frame;

	if (undistort)
	    cv::remap(frame, undist, map1, map2, cv::INTER_LINEAR);
	else
	    undist = frame;

	cv::warpPerspective(undist, warped, transform, undist.size());

	cv::cvtColor(warped, grey, cv::COLOR_BGR2GRAY);

	cv::flip(grey, grey, -1);
	
	quadratic(grey, highContrast, -128, 1.8, 0);
	cv::GaussianBlur(highContrast, highContrast, cv::Size(19,19), 2.0f);
	quadratic(highContrast, highContrast, -220, 2, 0);

	cv::imshow("Frame", highContrast);
	int key = cv::waitKey(10);
	if (key == 27)
	    return;
	else if (key != -1) {
	    player.play(highContrast);
	}
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void playImage(std::string imageFile, bool undistort,
	       cv::Mat map1, cv::Mat map2)
{
    ImagePlayer player(FREQ_MIN, FREQ_MAX, VOLUME, LINES, COLUMN_TIME);

    cv::Mat frame, undist;
    frame = cv::imread(imageFile);
    if (frame.empty()) {
	std::cerr << "ERROR: could not open '"
		  << imageFile
		  << "'" << std::endl;
	return;
    }

    if (undistort)
	    cv::remap(frame, undist, map1, map2, cv::INTER_LINEAR);
    else
	undist = frame;

    

    player.play(undist);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char** argv)
{
    int camera;
    std::string calibrationFile, imageFile;
    if (!parseArgs(argc, argv, camera, calibrationFile, imageFile)) {
	printHelp();
	return 1;
    }

    bool undistort = false;
    cv::Mat map1, map2;
    if (calibrationFile != "") {
	undistort = true;
	if (!loadMaps(calibrationFile, map1, map2)) {
	    return 2;
	}
    }

    if (camera != -1) {
	playCamera(camera, undistort, map1, map2);
    }
    else {
	playImage(imageFile, undistort, map1, map2);
    }

    return 0;
}
