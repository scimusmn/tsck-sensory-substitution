#include <iostream>
#include <vector>
#include <mutex>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include "ImagePlayer.hpp"

extern "C" {
#include "b64/base64.h"
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "ERROR: no image file provided; aborting!" << std::endl;
        return 0;
    }
    
    std::string filename(argv[1]);
    cv::Mat image = cv::imread(filename);

    ImagePlayer player(300, 700, 0.5, 100, 0.05);
    player.play(image);

    return 0;
}
