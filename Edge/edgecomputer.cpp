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
#include "Edge/Model/dag.h"
#include "Edge/callbackclient.h"
#include "Edge/edgeclientgrpc.h"
#include "Edge/edgemessages.h"
#include "Edge/stateclient.h"
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
      // - exception thrown: do not proceed with execution, ignore invocation
      // - last function: send final response to callback
      // - non-last function: invoke next function in the chain via companion

      if (lastFunction(myRequest)) {
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
        // functions to be invoked
        std::list<std::pair<size_t, std::string>> myFunctions;

        if (myRequest.chain_size() > 0) {
          // chain
          assert(((int)myRequest.nextfunctionindex() + 1) <
                 myRequest.chain_size());
          if (((int)myRequest.nextfunctionindex() + 1) <
              myRequest.chain_size()) {
            myFunctions.emplace_back(
                myRequest.nextfunctionindex() + 1,
                myRequest.chain(myRequest.nextfunctionindex() + 1));
          }

        } else {
          // DAG
          assert(myRequest.dag().names_size() > 0);
          assert((int)myRequest.nextfunctionindex() <
                 myRequest.dag().successors_size());
          if ((int)myRequest.nextfunctionindex() <
              myRequest.dag().successors_size()) {
            for (const auto myNdx :
                 myRequest.dag()
                     .successors(myRequest.nextfunctionindex())
                     .functions()) {
              assert((int)myNdx < myRequest.dag().names_size());
              if ((int)myNdx < myRequest.dag().names_size()) {
                myFunctions.emplace_back(myNdx, myRequest.dag().names(myNdx));
              }
            }
          }
        }

        const std::lock_guard<std::mutex> myLock(theParent.theCompanionMutex);
        if (theParent.theCompanionClient.get() == nullptr) {
          LOG(ERROR) << "companion not set for " << theParent.serverEndpoint();
          myFunctions.clear(); // do not invoke functions
        }
        const auto myCompanionEndpoint =
            theParent.theCompanionClient->serverEndpoint();

        // invoke all functions
        for (const auto& elem : myFunctions) {
          const auto myNewRequest = LambdaRequest(myRequest).regenerate(
              elem.second, elem.first, myResp);

          // send the next request, we expect immediate async response
          VLOG(3) << "invoking next function on " << myCompanionEndpoint << ", "
                  << myNewRequest;
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
                           const bool          aSecure,
                           const UtilCallback& aUtilCallback)
    : EdgeComputer(0, aServerEndpoint, aSecure, aUtilCallback) {
  // noop
}

EdgeComputer::EdgeComputer(const size_t        aNumThreads,
                           const std::string&  aServerEndpoint,
                           const bool          aSecure,
                           const UtilCallback& aUtilCallback)
    : EdgeServer(aServerEndpoint)
    , theSecure(aSecure)
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
    , theCompanionClient()
    , theCompanionMutex()
    , theStateClient()
    , theInvocations() {
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
  {
    const std::lock_guard<std::mutex> myLock(theMutex);
    if (theAsyncWorkers.get() == nullptr) {
      throw std::runtime_error(
          "cannot set the companion end-point of a synchronous edge computer");
    }
  }

  const std::lock_guard<std::mutex> myLock(theCompanionMutex);
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
  theCompanionClient =
      std::make_unique<EdgeClientGrpc>(aCompanionEndpoint, theSecure);
}

void EdgeComputer::state(const std::string& aStateEndpoint) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  if (aStateEndpoint.empty()) {
    LOG(WARNING) << "clearing the state end-point of " << serverEndpoint();
    theStateClient.reset();
    return;
  }
  if (theStateClient.get() == nullptr) {
    LOG(INFO) << "setting the state end-point of " << serverEndpoint() << " to "
              << aStateEndpoint;

  } else {
    LOG(WARNING) << "changing the state end-point of " << serverEndpoint()
                 << " from " << theStateClient->serverEndpoint() << " to "
                 << aStateEndpoint;
  }
  theStateClient = std::make_unique<StateClient>(aStateEndpoint);
}

rpc::LambdaResponse EdgeComputer::process(const rpc::LambdaRequest& aReq) {
  VLOG(3) << LambdaRequest(aReq);

  rpc::LambdaResponse myResp;

  std::string myRetCode = "OK";
  try {
    // it is an error to have both a chain and a DAG
    if (aReq.chain_size() > 0 and aReq.dag().names_size() > 0) {
      throw std::runtime_error(
          "Cannot specify both a function chain and a DAG in the request");
    }

    // it is an error not have the callback with a function chain or DAG
    // with more than one function
    if ((aReq.chain_size() > 1 or aReq.dag().names_size() > 1) and
        aReq.callback().empty()) {
      throw std::runtime_error("Cannot handle multiple functions without a "
                               "callback in the request");
    }

    // it is an error to request the execution of a function beyond the chain
    if (aReq.chain_size() > 0 and
        (int) aReq.nextfunctionindex() >= aReq.chain_size()) {
      throw std::runtime_error(
          "Out-of-range function execution requested in a chain");
    }

    // it is an error to have a DAG without the UUID set
    if (aReq.dag().names_size() > 1 and aReq.uuid().empty()) {
      throw std::runtime_error(
          "Cannot execute a DAG without a UUID in the request");
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
        if (checkPreconditions(aReq)) {
          theAsyncQueue->push(aReq);
        }
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

bool EdgeComputer::checkPreconditions(const rpc::LambdaRequest& aRequest) {
  // no DAG or single-function DAG or first function in a DAG: return now
  if (aRequest.dag().names_size() <= 1 or aRequest.nextfunctionindex() == 0) {
    return true;
  }

  // check how many invocations we expect according to the DAG
  size_t myExpected = 0;
  for (ssize_t i = 0; i < aRequest.dag().successors_size(); i++) {
    for (const auto& mySuccessor : aRequest.dag().successors(i).functions()) {
      if (mySuccessor == aRequest.nextfunctionindex()) {
        myExpected++;
      }
    }
  }

  // the function invoked has only one precedessor, proceed immediately
  if (myExpected == 1) {
    return true;
  }

  const std::lock_guard<std::mutex> myLock(theMutex);
  assert(not aRequest.uuid().empty());
  auto  it       = theInvocations.emplace(makeHash(aRequest), 0).first;
  auto& myActual = it->second;
  myActual++;
  assert(myActual <= myExpected);
  VLOG(3) << "hash " << makeHash(aRequest) << ": requested " << myExpected
          << " vs actual " << myActual << " invocations";
  if (myActual == myExpected) {
    theInvocations.erase(it);
    return true;
  }
  return false;
}

bool EdgeComputer::lastFunction(const rpc::LambdaRequest& aRequest) {
  if (
      // single function invocation
      (aRequest.chain_size() == 0 and aRequest.dag().names_size() == 0) or

      // last function in a chain
      (aRequest.chain_size() >= 1 and
       (int) aRequest.nextfunctionindex() == (aRequest.chain_size() - 1)) or

      // last function in a DAG
      (aRequest.dag().names_size() > 1 and
       (int) aRequest.nextfunctionindex() ==
           aRequest.dag().successors_size())) {
    return true;
  }
  return false;
}

std::string EdgeComputer::makeHash(const rpc::LambdaRequest& aRequest) {
  return std::to_string(aRequest.nextfunctionindex()) + "-" + aRequest.uuid();
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

  if (not handleRemoteStates(aReq, myResp)) {
    throw std::runtime_error("could not handle all the remote states");
  }

  VLOG(2) << "number of busy descriptors " << theDescriptors.size();
  theDescriptors.erase(myIt.first);

  return myResp;
}

bool EdgeComputer::handleRemoteStates(const rpc::LambdaRequest& aRequest,
                                      rpc::LambdaResponse& aResponse) const {
  for (const auto& elem : aRequest.states()) {
    // the state is embedded in the message
    if (elem.second.location().empty()) {
      continue;
    }

    // the state is stored on the local state server
    if (elem.second.location() == stateClient().serverEndpoint()) {
      continue;
    }

    // there are not state dependencies at all
    const auto jt = aRequest.dependencies().find(elem.first);
    if (jt == aRequest.dependencies().end()) {
      continue;
    }

    auto myDepends = false;
    for (ssize_t i = 0; i < jt->second.functions_size(); i++) {
      if (jt->second.functions(i) == aRequest.name()) {
        myDepends = true;
        break;
      }
    }
    // the current function does not depend on this state
    if (not myDepends) {
      continue;
    }

    // retrieve from remote server
    StateClient myStateClient(elem.second.location());
    std::string myContent;
    if (not myStateClient.Get(elem.first, myContent)) {
      throw std::runtime_error("could not find state in " +
                               elem.second.location() + ": " + elem.first);
    }

    // copy into local server
    stateClient().Put(elem.first, myContent);
    // delete from remote server
    const auto ret = myStateClient.Del(elem.first);
    LOG_IF(WARNING, ret == false)
        << "state removed during access on " << elem.second.location() << ": "
        << elem.first;

    // update location of the state on the response
    auto it = aResponse.mutable_states()->find(elem.first);
    if (it == aResponse.mutable_states()->end()) {
      throw std::runtime_error("could not find state in the response: " +
                               elem.first);
    }
    it->second.set_location(stateClient().serverEndpoint());
  }
  return true;
}

StateClient& EdgeComputer::stateClient() const {
  if (theStateClient.get() == nullptr) {
    throw std::runtime_error(
        "cannot handle remote states without a state server");
  }
  return *theStateClient;
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
