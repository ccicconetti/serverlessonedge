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

#include "facedetectcomputer.h"

#include "OpenCV/cvutils.h"
#include "Support/chrono.h"
#include "Support/tostring.h"

#include "opencv2/highgui.hpp"

#include <nlohmann/json.hpp>

#include <glog/logging.h>

#include <stdexcept>
#include <vector>

using json = nlohmann::json;

namespace uiiit {
namespace edge {

FaceDetectComputer::FaceDetectComputer(
    const std::string&                            aServerEndpoint,
    const size_t                                  aNumWorkers,
    const size_t                                  aNumThreads,
    const Models&                                 aModels,
    const double                                  aFaceDetectScale,
    const std::string&                            aDummyLambda,
    const std::function<std::array<double, 3>()>& aProcessLoadCallback)
    : EdgeServer(aServerEndpoint, aNumWorkers)
    , theNumOpenCvThreads(aNumThreads)
    , theClassifiers()
    , theModels(aModels)
    , theFaceDetectScale(aFaceDetectScale)
    , theDummyLambda(aDummyLambda)
    , theProcessLoadCallback(aProcessLoadCallback) {
  // limit the maximum number of threads used by the OpenCV library
  cv::setNumThreads(aNumThreads);

  if (aDummyLambda.empty()) {
    throw std::runtime_error("Invalid empty name for the dummy lambda");
  }
  for (const auto& myPair : aModels) {
    if (myPair.first == aDummyLambda) {
      throw std::runtime_error("Same name for the dummy and real lambda: " +
                               theDummyLambda);
    }
    if (myPair.first.empty()) {
      throw std::runtime_error("Invalid empty face detect lambda name");
    }
  }
}

void FaceDetectComputer::init() {
  const auto myThreadIds = threadIds();
  assert(not myThreadIds.empty());
  assert(theClassifiers.empty());

  for (const auto& myPair : theModels) {
    assert(myPair.first != theDummyLambda);
    assert(not myPair.first.empty());

    const auto it =
        theClassifiers.insert({myPair.first, Classifiers::mapped_type()});
    assert(it.second);
    std::ignore = it; // to suppress unused warning with NDEBUG

    for (const auto& myThreadId : myThreadIds) {
      VLOG(1) << "registering handler with thread id " << myThreadId;
      const auto jt =
          it.first->second.insert({myThreadId, cv::CascadeClassifier()});
      assert(jt.second);
      if (not jt.first->second.load(myPair.second)) {
        throw std::runtime_error("Invalid model file for lambda " +
                                 myPair.first + ": " + myPair.second);
      }
    }
  }
}

rpc::LambdaResponse
FaceDetectComputer::process(const rpc::LambdaRequest& aReq) {
  rpc::LambdaResponse myResp;

  std::string myRetCode = "OK";
  try {
    if (aReq.name() == theDummyLambda) {
      // do nothing

    } else {
      const auto it = theClassifiers.find(aReq.name());

      if (it != theClassifiers.end()) {
        const auto jt = it->second.find(std::this_thread::get_id());
        if (jt == it->second.end()) {
          LOG(FATAL) << "the impossible happened: job executed by a thread "
                        "with unregisted handler: " << std::this_thread::get_id();
        }
        faceDetect(jt->second, aReq, myResp);

      } else {
        myRetCode = "Unsupported lambda: " + aReq.name() +
                    " (supported lambdas: " +
                    toString(theClassifiers,
                             ",",
                             [](const Classifiers::value_type& aPair) {
                               return aPair.first;
                             }) +
                    ")";
      }
    }
  } catch (const std::exception& aErr) {
    myRetCode = aErr.what();
  } catch (...) {
    myRetCode = "Unknown error";
  }

  myResp.set_retcode(myRetCode);
  return myResp;
}

void FaceDetectComputer::faceDetect(cv::CascadeClassifier&    aClass,
                                    const rpc::LambdaRequest& aReq,
                                    rpc::LambdaResponse&      aResp) {

  double myPrepTime;
  double myClassTime;

  support::Chrono myChrono(true);

  // fill the load values in the response
  const auto myLoads = theProcessLoadCallback();
  aResp.set_load1(normalize(myLoads[0]));
  aResp.set_load10(normalize(myLoads[1]));
  aResp.set_load30(normalize(myLoads[2]));

  // copy image from the lambda request
  std::vector<char> myImgData(aReq.datain().size());
  for (size_t i = 0; i < aReq.datain().size(); i++) {
    myImgData[i] = aReq.datain()[i];
  }

  // decode image
  auto myImg = cv::imdecode(cv::Mat(myImgData), cv::IMREAD_COLOR);

  if (myImg.empty()) {
    throw std::runtime_error("Could not read image (size " +
                             std::to_string(aReq.datain().size()) + " bytes)");
  }

  const auto myFaces = cv::detectFaces(
      aClass, myImg, theFaceDetectScale, myPrepTime, myClassTime);

  VLOG(1) << "classified image size " << aReq.datain().size()
          << " bytes, preparation time: " << myPrepTime * 1e3 << " ms, "
          << "classification time: " << myClassTime * 1e3 << " ms, "
          << myFaces.size() << " objects detected";

  json myJson;
  myJson["objects"] = json::array();
  for (const auto& myRect : myFaces) {
    json myFace;
    myFace["x"] = myRect.x * theFaceDetectScale;
    myFace["y"] = myRect.y * theFaceDetectScale;
    myFace["w"] = myRect.width * theFaceDetectScale;
    myFace["h"] = myRect.height * theFaceDetectScale;
    myJson["objects"].push_back(myFace);
  }
  aResp.set_ptime(myChrono.stop() * 1e3 + 0.5);
  aResp.set_output(myJson.dump());
}

unsigned int FaceDetectComputer::normalize(const double aValue) const noexcept {
  return std::min(
      100u,
      static_cast<unsigned int>(0.5 + 100.0 * aValue / theNumOpenCvThreads));
}

} // namespace edge
} // namespace uiiit
