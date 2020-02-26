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

#include "cvutils.h"

#include "opencv2/imgproc.hpp"
#include "opencv2/opencv.hpp"

#include <stdexcept>
#include <vector>

namespace cv {

std::string toJpgString(InputArray aImg) {
  std::vector<unsigned char> myBuf;
  if (not imencode(".jpg", aImg, myBuf, {IMWRITE_JPEG_QUALITY, 100})) {
    throw std::runtime_error("Could not encode image as JPEG");
  }
  return std::string(reinterpret_cast<char*>(myBuf.data()), myBuf.size());
}

void addFacesEyes(Mat&                     aImg,
                  const std::vector<Rect>& aFaces,
                  const std::vector<Rect>& aEyes) {
  for (const auto& myFace : aFaces) {
    rectangle(aImg,
              Point(myFace.x, myFace.y),
              Point(myFace.x + myFace.width, myFace.y + myFace.height),
              Scalar(0, 255, 255), // yellow
              4);                  // line width
  }
  for (const auto& myEye : aEyes) {
    rectangle(aImg,
              Point(myEye.x, myEye.y),
              Point(myEye.x + myEye.width, myEye.y + myEye.height),
              Scalar(255, 0, 0), // blue
              4);                // line width
  }
}

void addElapsed(Mat& aImg, const double aElapsed) {
  int        myBaseline = 0;
  const auto myText =
      std::to_string(static_cast<long>(0.5 + aElapsed * 1e3)) + " ms";

  const auto myTextSize =
      getTextSize(myText, FONT_HERSHEY_SIMPLEX, 1.0, 2, &myBaseline);
  rectangle(aImg,
            Point(0, myBaseline),
            Point(myTextSize.width, myBaseline + myTextSize.height + 2),
            Scalar(0, 0, 0), // black
            FILLED);
  putText(aImg,
          myText,
          Point(0, myTextSize.height + myBaseline - 1),
          FONT_HERSHEY_SIMPLEX,
          1.0,
          Scalar(255, 255, 255), // white
          2,
          LINE_8,
          false);
}

std::vector<Rect> detectFaces(CascadeClassifier& aClassifier,
                              InputArray         aImg,
                              const double       aScale,
                              double&            aPrepTime,
                              double&            aClassTime) {
  uiiit::support::Chrono myChrono(true);

  // convert to gray scale and scale image
  Mat myGrayImg;
  Mat mySmallImg;
  cvtColor(aImg, myGrayImg, COLOR_BGR2GRAY);
  resize(myGrayImg,
         mySmallImg,
         Size(),
         1.0 / aScale,
         1.0 / aScale,
         INTER_LINEAR_EXACT);
  equalizeHist(mySmallImg, mySmallImg);

  aPrepTime = myChrono.restart();

  // detect faces
  std::vector<Rect> myFaces;
  aClassifier.detectMultiScale(mySmallImg,
                               myFaces,
                               1.1,
                               2,
                               0 //| CASCADE_FIND_BIGGEST_OBJECT
                                 //| CASCADE_DO_ROUGH_SEARCH
                                   | CASCADE_SCALE_IMAGE,
                               Size(30, 30));

  aClassTime = myChrono.stop();

  return myFaces;
}

} // namespace cv
