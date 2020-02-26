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

#include "OpenCV/cvutils.h"
#include "Support/checkfiles.h"
#include "Support/fileutils.h"

#include "opencv2/highgui.hpp"
#include "opencv2/objdetect.hpp"

#include <boost/program_options.hpp>

#include <glog/logging.h>

#include <iostream>
#include <set>
#include <string>

namespace po = boost::program_options;

int main(int argc, const char* argv[]) {
  po::options_description myDesc("Allowed options");
  std::string             myModel;
  std::string             mySave;
  std::string             myImage;
  double                  myScale;

  // clang-format off
  myDesc.add_options()
    ("help,h", "produce help message")
    ("scale",
     po::value<double>(&myScale)->default_value(1.0),
     "Image scale.")
    ("model",
     po::value<std::string>(&myModel)->default_value("haarcascade_frontalface_default.xml"),
     "Path of the file containing the training model.")
    ("save",
     po::value<std::string>(&mySave),
     "If not empty, save the faces to files with the specified basename.")
    ("image",
     po::value<std::string>(&myImage),
     "Path of the file containing the image.")
    ;
  // clang-format on

  try {
    po::variables_map myVarMap;
    po::store(po::parse_command_line(argc, argv, myDesc), myVarMap);
    po::notify(myVarMap);

    if (myVarMap.count("help")) {
      std::cout << myDesc << std::endl;
      return EXIT_FAILURE;
    }

    uiiit::support::checkFiles(std::set<std::string>({myModel, myImage}));

    // load the cascade classifier

    auto myClassifier = cv::CascadeClassifier();

    if (not myClassifier.load(myModel)) {
      throw std::runtime_error("Could not read model: " + myModel);
    }

    // decode the image from file into memory
    auto myImg = cv::imdecode(cv::Mat(uiiit::support::readFile(myImage)),
                              cv::IMREAD_COLOR);

    if (myImg.empty()) {
      throw std::runtime_error("Could not read image: " + myImage);
    }

    // detect faces
    double     myPrepTime;
    double     myClassTime;
    const auto myFaces =
        cv::detectFaces(myClassifier, myImg, myScale, myPrepTime, myClassTime);

    LOG(INFO) << "preparation time:    " << myPrepTime * 1e3 << " ms";
    LOG(INFO) << "classification time: " << myClassTime * 1e3 << " ms";
    LOG(INFO) << myFaces.size() << " faces detected";

    // print rectangles or save images depending on CLI argument --save
    size_t myCnt = 0;
    for (const auto& myRect : myFaces) {
      auto myFace = myRect;
      myFace.x *= myScale;
      myFace.y *= myScale;
      myFace.width *= myScale;
      myFace.height *= myScale;
      if (mySave.empty()) {
        std::cout << '#' << myCnt << ' ' << myFace.x << ' ' << myFace.y << ' '
                  << myFace.width << ' ' << myFace.height << std::endl;
      } else {
        const auto myOutFilename =
            mySave + "-" + std::to_string(myCnt) + ".png";
        if (not cv::imwrite(myOutFilename,
                            myImg(myFace),
                            {cv::IMWRITE_PNG_COMPRESSION, 9})) {
          throw std::runtime_error("Error while saving to " + myOutFilename);
        }
      }
      myCnt++;
    }

    return EXIT_SUCCESS;

  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Exception caught: " << aErr.what();

  } catch (...) {
    LOG(ERROR) << "Unknown exception caught";
  }
}
