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

#include "localoptimizerasync.h"

#include "forwardingtable.h"

#include "edgeserver.grpc.pb.h"

#include <glog/logging.h>

#include <cassert>

namespace uiiit {
namespace edge {

LocalOptimizerAsync::LocalOptimizerAsync(ForwardingTable& aForwardingTable,
                                         const double     aAlpha)
    : LocalOptimizer(aForwardingTable)
    , theAlpha(aAlpha)
    , theMutex()
    , theWeights()
    , theChrono(true) {
  LOG(INFO) << "Creating an async local optimizer, with alpha = " << aAlpha;
}

void LocalOptimizerAsync::operator()(const rpc::LambdaRequest& aReq,
                                     const std::string&        aDestination,
                                     const double              aTime) {
  assert(aTime > 0);

  const auto& myLambda    = aReq.name();
  auto        myCurWeight = aTime;

  VLOG(2) << "req " << myLambda << ", dest " << aDestination << ", time "
          << aTime << " s";

  const std::lock_guard<std::mutex> myLock(theMutex);

  const auto myNow = theChrono.time();
  const auto myRet =
      theWeights.insert({myLambda,
                         std::map<std::string, Elem>(
                             {{aDestination, Elem{myCurWeight, myNow}}})});
  if (not myRet.second) {
    const auto myInnerRet =
        myRet.first->second.insert({aDestination, Elem{myCurWeight, myNow}});
    if (not myInnerRet.second) {
      assert(myNow >= myInnerRet.first->second.theTimestamp);
      if (myNow - myInnerRet.first->second.theTimestamp < stalePeriod()) {
        myCurWeight *= (1 - theAlpha);
        myCurWeight += theAlpha * myInnerRet.first->second.theWeight;
      }
      myInnerRet.first->second.theTimestamp = myNow;
      myInnerRet.first->second.theWeight    = myCurWeight;
    }
  }

  theForwardingTable.change(myLambda, aDestination, myCurWeight);
}

} // namespace edge
} // namespace uiiit
