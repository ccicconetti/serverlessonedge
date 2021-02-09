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

#include "ptimeestimatorutil.h"

#include "Edge/edgemessages.h"

#include <glog/logging.h>

namespace uiiit {
namespace edge {

PtimeEstimatorUtil::PtimeEstimatorUtil(const size_t       aRttWindowSize,
                                       const double       aRttStalePeriod,
                                       const double       aUtilLoadTimeout,
                                       const size_t       aUtilWindowSize,
                                       const std::string& aOutput)
    : PtimeEstimator(Type::Util)
    , theRttEstimator(aRttWindowSize, aRttStalePeriod)
    , theUtilEstimator(aUtilLoadTimeout, aUtilWindowSize)
    // with timestap, with per-line flushing, truncate
    , theSaver(aOutput, true, true, false) {
  LOG_IF(INFO, not aOutput.empty())
      << "saving measurements to output file " << aOutput;
}

std::string PtimeEstimatorUtil::operator()(const rpc::LambdaRequest& aReq) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  const auto                        mySize = size(aReq);
  const auto                        ret    = theUtilEstimator.best(
      aReq.name(), mySize, theRttEstimator.rtts(aReq.name(), mySize));
  const auto it =
      theEstimates.emplace(reinterpret_cast<uint64_t>(&aReq),
                           Estimates{std::get<UtilEstimator::BT_RTT>(ret),
                                     std::get<UtilEstimator::BT_PTIME>(ret)});
  std::ignore = it; // ifdef NDEBUG
  assert(it.second);
  return std::get<UtilEstimator::BT_DEST>(ret);
}

void PtimeEstimatorUtil::processSuccess(const rpc::LambdaRequest& aReq,
                                        const std::string&        aDestination,
                                        const LambdaResponse&     aRep,
                                        const double              aTime) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  const auto myRtt = aTime - aRep.processingTimeSeconds();
  auto       it    = theEstimates.find(reinterpret_cast<uint64_t>(&aReq));
  if (it != theEstimates.end()) {
    theSaver(aReq.name() + " " + aDestination,
             size(aReq),
             aRep.theLoad1,
             it->second.theRtt,
             myRtt,
             it->second.thePtime,
             aRep.processingTimeSeconds());
    theEstimates.erase(it);
  } else {
    assert(false);
  }
  theUtilEstimator.add(aReq.name(),
                       aDestination,
                       size(aReq),
                       aRep.processingTimeSeconds(),
                       aRep.theLoad1,
                       aRep.theLoad10);
  theRttEstimator.add(aReq.name(), aDestination, size(aReq), myRtt);
}

void PtimeEstimatorUtil::privateAdd(const std::string& aLambda,
                                    const std::string& aDestination) {
  ASSERT_IS_LOCKED(theMutex);
  theUtilEstimator.add(aLambda, aDestination);
  theRttEstimator.add(aLambda, aDestination);
}

void PtimeEstimatorUtil::privateRemove(const std::string& aLambda,
                                       const std::string& aDestination) {
  ASSERT_IS_LOCKED(theMutex);
  theUtilEstimator.remove(aLambda, aDestination);
  theRttEstimator.remove(aLambda, aDestination);
}
} // namespace edge
} // namespace uiiit
