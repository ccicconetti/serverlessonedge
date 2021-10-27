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

#include "Edge/edgecomputer.h"

#include "Edge/Model/chain.h"
#include "Edge/callbackclient.h"
#include "Edge/edgeclientgrpc.h"
#include "Edge/edgemessages.h"
#include "Support/threadpool.h"

#include <glog/logging.h>
#include <grpc++/grpc++.h>

#include <mutex>

namespace uiiit {
namespace edge {

EdgeComputer::AsyncWorker::AsyncWorker(
    EdgeComputer& aParent, support::Queue<rpc::LambdaRequest>& aQueue)
    : theParent(aParent)
    , theQueue(aQueue) {
  // noop
}

void EdgeComputer::AsyncWorker::operator()() {
  while (true) {
    try {
      // retrieve one of the pending function requests
      auto myRequest = theQueue.pop();

      // executes the lambda function (blocks)
      auto myResp = theParent.blockingExecution(myRequest);

      // what follows depends on whether this is the last function
      // to be executed in the chain or not
      //
      // last function: send final response to callback
      // non-last function: invoke the next function in the chain via companion

      if (myRequest.chain_size() == 0 or
          (int) myRequest.nextfunctionindex() == (myRequest.chain_size() - 1)) {
        myResp.set_responder(theParent.serverEndpoint());
        myResp.set_hops(myRequest.hops() + 1);
        myResp.set_retcode("OK");

        // send the response to the callback server indicated in the request
        CallbackClient myClient(myRequest.callback());
        LambdaResponse myResponse(myResp);
        myResponse.removePtimeLoad();
        VLOG(3) << "sending response to " << myRequest.callback() << ", "
                << myResponse;
        myClient.ReceiveResponse(myResponse);

      } else {
        // prepare the new request
        const auto myName =
            (((int)myRequest.nextfunctionindex() + 1) <
             myRequest.chain_size()) ?
                myRequest.chain(myRequest.nextfunctionindex() + 1) :
                std::string(); // empty name will result in an error
        LambdaRequest myNewRequest(myName, myResp.output(), myResp.dataout());
        myNewRequest.theHops = myResp.hops() + 1;

        LambdaRequest myOldRequest(myRequest);
        myNewRequest.theStates   = myOldRequest.theStates;
        myNewRequest.theCallback = myOldRequest.theCallback;
        myNewRequest.theChain    = std::move(myOldRequest.theChain);
        myNewRequest.theNextFunctionIndex =
            myOldRequest.theNextFunctionIndex + 1;

        {
          const std::lock_guard<std::mutex> myLock(theParent.theMutex);
          if (theParent.theCompanionClient.get() == nullptr) {
            LOG(ERROR) << "companion not set for "
                       << theParent.serverEndpoint();
          } else {
            // send the next request, we expect immediate async response
            const auto myCompanionEndpoint =
                theParent.theCompanionClient->serverEndpoint();
            VLOG(3) << "invoking next function on " << myCompanionEndpoint
                    << ", " << myNewRequest;
            const auto myImmediateResp =
                theParent.theCompanionClient->RunLambda(myNewRequest, false);
            LOG_IF(ERROR, myImmediateResp.theRetCode != "OK")
                << "error when executing the next function in the chain via "
                << myCompanionEndpoint << ": " << myImmediateResp.theRetCode;
            LOG_IF(ERROR, not myImmediateResp.theAsynchronous)
                << "received a synchronous response when executing the next "
                   "function in the chain via "
                << myCompanionEndpoint << ": result ignored";
          }
        }
      }

    } catch (const support::QueueClosed&) {
      break;
    } catch (const std::exception& aErr) {
      LOG(ERROR) << "exception thrown: " << aErr.what();
    } catch (...) {
      LOG(ERROR) << "unknown exception thrown";
    }
  }
}

void EdgeComputer::AsyncWorker::stop() {
  // noop
}

EdgeComputer::EdgeComputer(const std::string&  aServerEndpoint,
                           const UtilCallback& aUtilCallback)
    : EdgeComputer(0, aServerEndpoint, aUtilCallback) {
  // noop
}

EdgeComputer::EdgeComputer(const size_t        aNumThreads,
                           const std::string&  aServerEndpoint,
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
    , theDescriptors()
    , theAsyncWorkers(aNumThreads == 0 ? nullptr :
                                         std::make_unique<WorkersPool>())
    , theAsyncQueue(aNumThreads == 0 ?
                        nullptr :
                        std::make_unique<support::Queue<rpc::LambdaRequest>>())
    , theCompanionClient() {
  if (aNumThreads > 0) {
    assert(theAsyncWorkers.get() != nullptr);
    assert(theAsyncQueue.get() != nullptr);
    LOG(INFO) << "Creating an asynchronous edge computer at end-point "
              << aServerEndpoint << ", with " << aNumThreads << " threads";
    for (size_t i = 0; i < aNumThreads; i++) {
      theAsyncWorkers->add(
          std::make_unique<AsyncWorker>(*this, *theAsyncQueue));
    }
    theAsyncWorkers->start();
  } else {
    LOG(INFO) << "Creating a sync-only edge computer at end-point "
              << aServerEndpoint;
  }
}

EdgeComputer::~EdgeComputer() {
  if (theAsyncWorkers.get() != nullptr) {
    assert(theAsyncQueue.get() != nullptr);
    theAsyncQueue->close();
    theAsyncWorkers->stop();
    theAsyncWorkers->wait();
  }
}

void EdgeComputer::companion(const std::string& aCompanionEndpoint) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  if (theAsyncWorkers.get() == nullptr) {
    throw std::runtime_error(
        "cannot set the companion end-point of a synchronous edge computer");
  }
  if (aCompanionEndpoint.empty()) {
    LOG(WARNING) << "clearing the companion end-point of " << serverEndpoint();
    theCompanionClient.reset();
    return;
  }
  if (theCompanionClient.get() == nullptr) {
    LOG(INFO) << "setting the companion end-point of " << serverEndpoint()
              << " to " << aCompanionEndpoint;

  } else {
    LOG(WARNING) << "changing the companion end-point of " << serverEndpoint()
                 << " from " << theCompanionClient->serverEndpoint() << " to "
                 << aCompanionEndpoint;
  }
  theCompanionClient = std::make_unique<EdgeClientGrpc>(aCompanionEndpoint);
}

rpc::LambdaResponse EdgeComputer::process(const rpc::LambdaRequest& aReq) {
  VLOG(3) << LambdaRequest(aReq);

  rpc::LambdaResponse myResp;

  std::string myRetCode = "OK";
  try {
    // it is an error not have the callback with a function chain
    if (aReq.chain_size() > 0 and aReq.callback().empty()) {
      throw std::runtime_error(
          "Cannot handle a function chain without a callback in the request");
    }

    // it is an error to request the execution of a function beyond the chain
    if (aReq.chain_size() > 0 and
        (int) aReq.nextfunctionindex() >= aReq.chain_size()) {
      throw std::runtime_error(
          "Out-of-range function execution requested in a chain");
    }

    // it is an error to request the execution of a function chain without
    // having set the companion beforehand, if needed
    if (aReq.chain_size() > 0 and
        (int) aReq.nextfunctionindex() < (aReq.chain_size() - 1) and
        theCompanionClient.get() == nullptr) {
      throw std::runtime_error(
          "Cannot invoke the next function in the chain without a companion");
    }

    if (aReq.dry()) {
      // dry run: just give an estimate of the time required to run the lambda
      // unlike the actual execution of a lambda, this operation is
      // synchronous

      // convert seconds to milliseconds
      std::array<double, 3> myLastUtils;
      myResp.set_ptime(
          0.5 + theComputer.simTask(LambdaRequest(aReq), myLastUtils) * 1e3);
      myResp.set_load1(0.5 + myLastUtils[0] * 100);
      myResp.set_load10(0.5 + myLastUtils[1] * 100);
      myResp.set_load30(0.5 + myLastUtils[2] * 100);

    } else if (not aReq.callback().empty()) {
      // asynchronous function request
      if (theAsyncWorkers.get() == nullptr) {
        myRetCode = "Cannot handle an asynchronous function invocation";
      } else {
        myResp.set_asynchronous(true);
        theAsyncQueue->push(aReq);
      }

    } else {
      // actual execution of the lambda function
      myResp = blockingExecution(aReq);
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

rpc::LambdaResponse
EdgeComputer::blockingExecution(const rpc::LambdaRequest& aReq) {
  // the task must be added outside the critical section below
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
  auto myResp = myDescriptor.theResponse->toProtobuf();
  myResp.set_ptime(myDescriptor.theChrono.stop() * 1e3 + 0.5); // to ms

  VLOG(2) << "number of busy descriptors " << theDescriptors.size();
  theDescriptors.erase(myIt.first);

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
