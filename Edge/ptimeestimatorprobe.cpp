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

#include "ptimeestimatorprobe.h"

#include "Edge/edgemessages.h"
#include "Support/conf.h"

#include <glog/logging.h>

#include <limits>

namespace uiiit {
namespace edge {

PtimeEstimatorProbe::PtimeEstimatorProbe(const size_t       aMaxClients,
                                         const std::string& aOutput)
    : PtimeEstimator(Type::Probe)
    , theClients(support::Conf("type=grpc"), aMaxClients)
    , theDestinations([](const std::string&, const std::string&) {
      return std::make_unique<Descriptor>();
    }) // with timestap, with per-line flushing, truncate
    , theSaver(aOutput, true, true, false) {
  LOG_IF(INFO, not aOutput.empty())
      << "saving measurements to output file " << aOutput;
}

std::string PtimeEstimatorProbe::operator()(const rpc::LambdaRequest& aReq) {
  const std::lock_guard<std::mutex> myLock(theMutex);

  const auto myAllDestinations =
      theDestinations.all(aReq.name(), [](Descriptor&) { return 0.0f; });

  std::string  myBestDestination;
  unsigned int myBestPtime = std::numeric_limits<unsigned int>::max();
  const auto   myReq       = LambdaRequest(aReq);
  for (const auto& myDestination : myAllDestinations) {
    const auto ret = theClients(myDestination.first, myReq, true); // dry
    VLOG(2) << "destination " << myDestination.first << ", simulated ptime "
            << ret.first.theProcessingTime << " ms";

    if (ret.first.theRetCode == "OK" and
        ret.first.theProcessingTime < myBestPtime) {
      myBestPtime       = ret.first.theProcessingTime;
      myBestDestination = myDestination.first;
    }
  }
  assert(not myBestDestination.empty());
  assert(myBestPtime < std::numeric_limits<unsigned int>::max());

  // add to the estimates
  const auto it =
      theEstimates.emplace(reinterpret_cast<uint64_t>(&aReq),
                           Estimates{0.0f, static_cast<float>(myBestPtime)});
  std::ignore = it; // ifdef NDEBUG
  assert(it.second);

  return myBestDestination;
}

void PtimeEstimatorProbe::processSuccess(const rpc::LambdaRequest& aReq,
                                         const std::string&        aDestination,
                                         const LambdaResponse&     aRep,
                                         const double              aTime) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  auto it = theEstimates.find(reinterpret_cast<uint64_t>(&aReq));
  if (it != theEstimates.end()) {
    theSaver(aReq.name() + " " + aDestination,
             size(aReq),
             it->second.thePtime,
             aRep.theProcessingTime);
    theEstimates.erase(it);
  } else {
    assert(false);
  }
}

void PtimeEstimatorProbe::privateAdd(const std::string& aLambda,
                                     const std::string& aDestination) {
  ASSERT_IS_LOCKED(theMutex);
  theDestinations.add(aLambda, aDestination);
}

void PtimeEstimatorProbe::privateRemove(const std::string& aLambda,
                                        const std::string& aDestination) {
  ASSERT_IS_LOCKED(theMutex);
  theDestinations.add(aLambda, aDestination);
}
} // namespace edge
} // namespace uiiit
