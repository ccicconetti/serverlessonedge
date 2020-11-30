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

#include "edgecomputer.h"

#include "edgemessages.h"

#include <glog/logging.h>
#include <grpc++/grpc++.h>

namespace uiiit {
namespace edge {

EdgeComputer::EdgeComputer(const std::string&  aServerEndpoint,
                           const UtilCallback& aUtilCallback)
    : EdgeServer(aServerEndpoint)
    , theComputer(
          "computer@" + aServerEndpoint,
          [this](const uint64_t                               aId,
                 const std::shared_ptr<const LambdaResponse>& aResponse) {
            taskDone(aId, aResponse);
          },
          aUtilCallback)
    , theDescriptorsCv()
    , theDescriptors() {
}

rpc::LambdaResponse EdgeComputer::process(const rpc::LambdaRequest& aReq) {
  rpc::LambdaResponse myResp;

  std::string myRetCode = "OK";
  try {
    if (aReq.dry()) {
      //
      // dry run: just give an estimate of the time required to run the lambda
      // unlike the actual execution of a lambda, this operation is synchronous
      //

      // convert seconds to milliseconds
      std::array<double, 3> myLastUtils;
      myResp.set_ptime(
          0.5 + theComputer.simTask(LambdaRequest(aReq), myLastUtils) * 1e3);
      myResp.set_load1(0.5 + myLastUtils[0] * 100);
      myResp.set_load10(0.5 + myLastUtils[1] * 100);
      myResp.set_load30(0.5 + myLastUtils[2] * 100);

    } else {
      //
      // actual execution of the lambda function
      //

      // the task must be added out of the critical section below
      // to avoid deadlock due to a race condition on tasks
      // that are very short (and become completed before
      // a new descriptor is added to theDescriptors)
      const auto myId = theComputer.addTask(LambdaRequest(aReq));

      std::unique_lock<std::mutex> myLock(theMutex);

      auto myNewDesc = std::make_unique<Descriptor>();

      const auto myIt = theDescriptors.insert(std::make_pair(myId, nullptr));
      assert(myIt.second);
      myIt.first->second = std::move(myNewDesc);
      theDescriptorsCv.notify_one();
      auto& myDescriptor = *myIt.first->second;

      // wait until we get a response
      myDescriptor.theCondition.wait(
          myLock, [&myDescriptor]() { return myDescriptor.theDone; });

      assert(myDescriptor.theResponse);
      myResp = myDescriptor.theResponse->toProtobuf();
      myResp.set_ptime(myDescriptor.theChrono.stop() * 1e3 + 0.5); // to ms

      VLOG(2) << "number of busy descriptors " << theDescriptors.size();
      theDescriptors.erase(myIt.first);
    }

  } catch (const std::exception& aErr) {
    myRetCode = aErr.what();
  } catch (...) {
    myRetCode = "Unknown error";
  }

  myResp.set_hops(aReq.hops() + 1);
  myResp.set_retcode(myRetCode);
  return myResp;
}

void EdgeComputer::taskDone(
    const uint64_t                               aId,
    const std::shared_ptr<const LambdaResponse>& aResponse) {
  VLOG(2) << "task " << aId << " done: " << aResponse->theRetCode;

  std::unique_lock<std::mutex> myLock(theMutex);

  // wait until the descriptor has been inserted into the map
  // this wait should be null in most cases and always very short:
  // consider a spinlock instead of a condition variable
  theDescriptorsCv.wait(
      myLock, [this, aId]() { return theDescriptors.count(aId) == 1; });

  const auto myIt = theDescriptors.find(aId);
  assert(myIt != theDescriptors.end());

  assert(static_cast<bool>(myIt->second));
  myIt->second->theResponse = aResponse;
  myIt->second->theDone     = true;
  myIt->second->theCondition.notify_one();
}

} // namespace edge
} // namespace uiiit
