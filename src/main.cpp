#include <iostream>
#include <vector>
#include <mutex>

#if WIN32
  #include <windows.h>
#else
  #include <X11/Xlib.h>
#endif

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>

#include "ArduinoSerial.hpp"
#include "ImagePlayer.hpp"

extern "C" {
#include <unistd.h>
#include "b64/base64.h"
}

// ugly globals!!
bool buttonPressed, playing;

#define FREQ_MIN 200.0
#define FREQ_MAX 1000.0
#define VOLUME 0.5
#define LINES 80
double COLUMN_TIME=0.01;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void printHelp()
{
    std::cout << "Usage: substitution [OPTIONS] [image file]" << std::endl
	      << "  -h        Show this help message and exit." << std::endl
	      << "  -f FILE   Calibration file to use for un-distortion. Omitting this option" << std::endl
	      << "            disables un-distortion." << std::endl
	      << "  -c ID     Camera to use. This option causes the program to ignore any image" << std::endl
	      << "            file that may have been passed." << std::endl
	      << "  -t TIME   The column time to use."
	      << std::endl;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void getScreenSize(int& width, int& height) {
#if WIN32
    width  = (int) GetSystemMetrics(SM_CXSCREEN);
    height = (int) GetSystemMetrics(SM_CYSCREEN);
#else
    Display* disp = XOpenDisplay(NULL);
    Screen*  scrn = DefaultScreenOfDisplay(disp);
    width  = scrn->width;
    height = scrn->height;
#endif
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

    while( (c = getopt(argc, argv, "ht:c:f:")) != -1) {
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

	case 't':
	    try { COLUMN_TIME = std::stof(optarg); }
	    catch(std::invalid_argument err) {
		std::cerr << "ERROR: '"
			  << optarg
			  << "' is not a valid number!"
			  << std::endl;
		return false;
	    }
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
    // open arduino port
    ArduinoSerial arduino;
    buttonPressed = false;
    playing = false;
    arduino.setDataCallback
	([](std::string key, std::string value) {
	    if (key == "button" && value == "1" && !playing)
		buttonPressed = true;
	    else
		buttonPressed = false;
	});

    try {
	auto portList = arduino.findMatchingPorts(METRO_MINI_VID, METRO_MINI_PID);
	if (portList.size() > 0)
	    arduino.openPort(portList[0], 115200);
	else {
	    std::cerr << "FATAL: could not find arduino!" << std::endl;
	    return;
	}
    }
    catch(std::runtime_error error) {
	std::cerr << "FATAL: " << error.what() << std::endl;
	return;
    }

    // open camera
    cv::VideoCapture camera(cameraId);
    if (!camera.isOpened()) {
	std::cerr << "ERROR: could not open camera '"
		  << cameraId
		  << "'" << std::endl;
	return;
    }

    int screenWidth, screenHeight;
    getScreenSize(screenWidth, screenHeight);

    std::string windowName = "Sensory Substitution";
    cv::namedWindow(windowName, cv::WINDOW_NORMAL);
    cv::setWindowProperty(windowName, cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
    
    ImagePlayer player(FREQ_MIN, FREQ_MAX, VOLUME, LINES, COLUMN_TIME);

    cv::Mat frame, undist, warped, transform, grey, highContrast, display;
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

	cv::resize(highContrast, display, cv::Size(screenWidth, screenHeight), 0, 0, cv::INTER_LINEAR);
	
	cv::imshow(windowName, display);

	int key = cv::waitKey(10);
	if (key == 27)
	    return;

	arduino.update();
	if (buttonPressed) {
	    buttonPressed = false;
	    playing = true;
	    player.play(highContrast, screenWidth, screenHeight, windowName);
	    playing = false;
	}
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void playImage(std::string imageFile, bool undistort,
	       cv::Mat map1, cv::Mat map2)
{
    ImagePlayer player(FREQ_MIN, FREQ_MAX, VOLUME, LINES, COLUMN_TIME);

    int screenWidth, screenHeight;
    getScreenSize(screenWidth, screenHeight);

    std::string windowName = "Sensory Substitution";
    cv::namedWindow(windowName, cv::WINDOW_NORMAL);
    cv::setWindowProperty(windowName, cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
    
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

    player.play(undist, screenWidth, screenHeight, windowName);
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
