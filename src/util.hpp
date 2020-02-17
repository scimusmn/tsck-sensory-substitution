#ifndef SENSORY_SUBSTITUTION_UTIL_HPP
#define SENSORY_SUBSTITUTION_UTIL_HPP

#include <opencv2/core.hpp>

#include "callbacks.hpp"
#include "smmServer.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/*! @brief Send an OpenCV matrix in response to an HTTP message.
 *
 * The image is returned as a base64-encoded JPEG.
 * @param frame The matrix to send.
 * @param m The HTTP message to send the matrix as a response to.
 */
void sendMat(cv::Mat& frame, httpMessage& m);

/*! @brief Build a mask from a given matrix and thresholds.
 *
 * @param frame The matrix to build the mask from.
 * @param s The thresholdSettings struct specifying how to build the mask.
 *
 * @returns The requested matrix.
 */
cv::Mat getMask(cv::Mat& frame, struct thresholdSettings s);

/*! @brief Load settings from a file.
 *
 * The file to load settings from is specified as the settingsFile member
 * of the given struct glob, and the settings loaded are stored in 
 * the ballSettings and bgSettings of the same.
 * @param g The struct specifying where to load the files, and where
 *          to store the results.
 * 
 * @returns True if the load was successful and false otherwise.
 */
bool loadSettings(struct glob* g);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#endif

/*                        ,d88b.d88b,
                          88888888888
                          `Y8888888Y'
                            `Y888Y'
                              `Y'                                */
