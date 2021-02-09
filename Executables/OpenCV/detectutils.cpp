/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2021 C. Cicconetti <https://ccicconetti.github.io/>

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

#include "detectutils.h"

#include "Edge/edgeclientinterface.h"
#include "OpenCV/cvutils.h"
#include "Support/chrono.h"
#include "Support/tostring.h"

#include "opencv2/objdetect.hpp"

#include <nlohmann/json.hpp>

#include <glog/logging.h>

using json = nlohmann::json;

namespace cv {

std::pair<double, unsigned short>
detect(uiiit::edge::EdgeClientInterface& aClient,
       const std::string&                aLambdaFaces,
       const std::string&                aLambdaEyes,
       Mat&                              aImg,
       std::vector<Rect>&                aFaces,
       const bool                        aDetectEyes,
       std::vector<Rect>&                aEyes,
       const bool                        aNormal) {
  aFaces.clear();
  aEyes.clear();

  // encode the image as a string in JPEG format
  const auto myImgString = toJpgString(aImg);

  // start chronometer to measure overall service latency
  uiiit::support::Chrono myChrono(true);

  // send lambda request to edge computing domain for face recognition
  const auto myResp = aClient.RunLambda(
      uiiit::edge::LambdaRequest(aLambdaFaces, "", myImgString), false);

  // exit immediately if there were errors
  if (myResp.theRetCode != "OK") {
    throw std::runtime_error("error: " + myResp.theRetCode);
  }

  VLOG(1) << "face recognition: " << toString(myResp) << ", elapsed "
          << (1e3 * myChrono.time() + 0.5) << " ms";

  // read the rectangles delimiting faces in the image
  auto myJson = json::parse(myResp.theOutput);

  for (const auto& myRect : myJson["objects"]) {
    aFaces.push_back(Rect(static_cast<int>(myRect["x"]),
                          static_cast<int>(myRect["y"]),
                          static_cast<int>(myRect["w"]),
                          static_cast<int>(myRect["h"])));
  }

  // detect eyes for each image, if enabled
  if (aDetectEyes) {
    std::vector<Rect> myNewFaces;
    for (const auto& myFace : aFaces) {
      // send a lambda request for each face detected
      uiiit::edge::LambdaRequest myReq(
          aLambdaEyes, "", toJpgString(aImg(myFace)));
      const auto myResp = aClient.RunLambda(myReq, false);

      // exit immediately if there were errors
      if (myResp.theRetCode != "OK") {
        throw std::runtime_error("error: " + myResp.theRetCode);
      }

      VLOG(1) << "eyes recognition: " << toString(myResp) << ", elapsed "
              << (1e3 * myChrono.time() + 0.5) << " ms";

      // read the rectangles delimiting eyes in the faces
      auto myJson = json::parse(myResp.theOutput);

      // detect normal faces (ie. with two eyes)
      if (not aNormal or myJson["objects"].size() == 2) {
        myNewFaces.push_back(myFace);
      } else {
        continue;
      }

      for (const auto& myRect : myJson["objects"]) {
        aEyes.push_back(Rect(myFace.x + static_cast<int>(myRect["x"]),
                             myFace.y + static_cast<int>(myRect["y"]),
                             static_cast<int>(myRect["w"]),
                             static_cast<int>(myRect["h"])));
      }
    }
    aFaces.swap(myNewFaces);
  }

  return std::make_pair(myChrono.stop(), myResp.theLoad1);
}

} // namespace cv
