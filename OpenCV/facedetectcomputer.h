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

#pragma once

#include "Edge/edgecontrollermessages.h"
#include "Edge/edgeserver.h"

#include "opencv2/objdetect.hpp"

#include <array>
#include <cassert>
#include <functional>
#include <map>
#include <string>

namespace uiiit {
namespace edge {

class FaceDetectComputer final : public EdgeServer
{
  // map of
  //   key: lambda function name
  //   value: map of
  //     key: identifier of the thread performing processing
  //     value: classifier for object detection
  using Classifiers =
      std::map<std::string, std::map<std::thread::id, cv::CascadeClassifier>>;

  // map of
  //   key: lambda function name
  //   value: path of the file containing the classification model
  using Models = std::map<std::string, std::string>;

 public:
  /**
   * \param aServerEndpoint the end-point of the gRPC server.
   * \param aNumWorkers the number of workers to spawn.
   * \param aNumThreads the number of threads used by the OpenCV library.
   * \param aModels the classification models, one for each lambda function.
   * \param aFaceDetectScale the object detection scale parameter.
   * \param aDummyLambda the name of a dummy lambda function that always
   * returns success immediately, which can be used to verify if the
   * computer is responsive.
   * \param aProcessLoadCallback the function to call to fill the
   * last 1-second and last 30-seconds loads in the lambda response,
   * invoked _before_ the actual processing happens.
   *
   * \throw std::runtime_error if a lambda function name (real or dummy)
   * is empty or if any real lambda function has the same as the dummy one
   */
  explicit FaceDetectComputer(
      const std::string&                            aServerEndpoint,
      const size_t                                  aNumWorkers,
      const size_t                                  aNumThreads,
      const Models&                                 aModels,
      const double                                  aFaceDetectScale,
      const std::string&                            aDummyLambda,
      const std::function<std::array<double, 3>()>& aProcessLoadCallback);

 private:
  /**
   * Perform initialization of the classifiers, one per lambda per thread.
   *
   * \throw std::runtime_error if the initialiation of the classifiers fail
   * for any reason.
   */
  void init() override;

  //! Check the lambda request and run the appropriate lambda function.
  rpc::LambdaResponse process(const rpc::LambdaRequest& aReq) override;

  //! Perform face detection on the given image.
  void faceDetect(cv::CascadeClassifier&    aClass,
                  const rpc::LambdaRequest& aReq,
                  rpc::LambdaResponse&      aResp);

  //! Normalize the given load value on the number of threads.
  unsigned int normalize(const double aValue) const noexcept;

 private:
  const size_t                                 theNumOpenCvThreads;
  Classifiers                                  theClassifiers;
  const Models                                 theModels;
  const double                                 theFaceDetectScale;
  const std::string                            theDummyLambda;
  const std::function<std::array<double, 3>()> theProcessLoadCallback;
};

} // namespace edge
} // namespace uiiit
