/*
 ___ ___ __     __ ____________
|   |   |  |   |__|__|__   ___/   Ubiquitout Internet @ IIT-CNR
|   |   |  |  /__/  /  /  /    C++ edge computing libraries and tools
|   |   |  |/__/  /   /  /  https://bitbucket.org/ccicconetti/edge_computing/
|_______|__|__/__/   /__/

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2018 Claudio Cicconetti <https://about.me/ccicconetti>

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Support/chrono.h"

#include "opencv2/objdetect.hpp"

namespace cv {

//! Encode an image as a full-quality JPEG into a std::string.
std::string toJpgString(InputArray aImg);

//! Add rectangles around faces and eyes.
void addFacesEyes(Mat&                         aImg,
                  const std::vector<cv::Rect>& aFaces,
                  const std::vector<cv::Rect>& aEyes);

//! Add time elapsed.
void addElapsed(Mat& aImg, const double aElapsed);

/**
 * Detect objects in an image using a cv::CascadeClassifier.
 *
 * \param aClassifier The classifier to use.
 * \param aImg The image to classify.
 * \param aScale The scale parameter to use.
 * \param aPrepTime The time to prepare the image for classification, in s.
 * \param aClassTime The classification time, in s.
 * \return the faces detected.
 */
std::vector<cv::Rect> detectFaces(CascadeClassifier& aClassifier,
                                  InputArray         aImg,
                                  const double       aScale,
                                  double&            aPrepTime,
                                  double&            aClassTime);
} // namespace cv
