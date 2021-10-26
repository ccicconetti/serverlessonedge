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

#include "client.h"

#include "Edge/edgeclientfactory.h"
#include "Edge/edgeclientinterface.h"
#include "Edge/edgemessages.h"
#include "Support/saver.h"
#include "Support/stat.h"
#include "Support/tostring.h"

#include <glog/logging.h>

#include <thread>

namespace uiiit {
namespace simulation {

Client::Client(const size_t                 aSeedUser,
               const size_t                 aSeedInc,
               const size_t                 aNumRequests,
               const std::set<std::string>& aServers,
               const support::Conf&         aClientConf,
               const std::string&           aLambda,
               const support::Saver&        aSaver,
               const bool                   aDry)
    : theSeedUser(aSeedUser)
    , theSeedInc(aSeedInc)
    , theMutex()
    , theSleepCondition()
    , theExitCondition()
    , theStopFlag(false)
    , theFinishedFlag(false)
    , theNotStartedFlag(true)
    , theLambdaChrono(false)
    , theClient(edge::EdgeClientFactory::make(aServers, aClientConf))
    , theNumRequests(aNumRequests)
    , theLambda(aLambda)
    , theSaver(aSaver)
    , theLatencyStat()
    , theProcessingStat()
    , theDry(aDry)
    , theSendRequestQueue()
    , theSizeDist()
    , theSizes()
    , theChain(nullptr)
    , theStates()
    , theCallback()
    , theContent() {
  LOG(INFO) << "created a client with seed (" << aSeedUser << "," << aSeedInc
            << "), which will send max " << aNumRequests << " requests to "
            << toString(aServers, ",") << ", "
            << (aLambda.empty() ?
                    std::string("function chain mode") :
                    (std::string("single function mode (") + aLambda + ")"))
            << (aDry ? ", dry run" : "");
}

Client::~Client() {
  stop();
}

void Client::operator()() {
  if (theNotStartedFlag == false) {
    throw std::runtime_error("client started multiple times");
  }
  theNotStartedFlag = false;
  try {
    for (size_t myCounter = 0; myCounter < theNumRequests; /* incr in body */) {
      {
        const std::lock_guard<std::mutex> myLock(theMutex);
        if (theStopFlag) {
          VLOG(2) << "client execution interrupted after sending " << myCounter
                  << " requests out of " << theNumRequests;
          break;
        }
      }

      myCounter += loop();
    }
  } catch (...) {
    finish();
    throw;
  }
  finish();
}

void Client::finish() {
  const std::lock_guard<std::mutex> myLock(theMutex);
  theFinishedFlag = true;
  theExitCondition.notify_one();
}

std::unique_ptr<edge::LambdaResponse>
Client::singleExecution(const std::string& aInput) {
  assert(not theLambda.empty() or not theCallback.empty());
  if (theChain.get() != nullptr and theCallback.empty()) {
    throw std::runtime_error("uninitialized callback end-point");
  }

  // create a new lambda request and copy in it the states and chain (if any)
  const auto myName =
      (theChain.get() != nullptr and theChain->functions().size() > 0) ?
          theChain->functions().front() :
          theLambda;
  edge::LambdaRequest myReq(myName, aInput);
  for (const auto& myState : theChain->allStates(false)) {
    const auto it = theStates.find(myState);
    assert(it != theStates.end());
    myReq.states().emplace(myState, edge::State(it->second));
  }
  if (theChain) {
    myReq.theChain = std::make_unique<edge::model::Chain>(*theChain);
  }
  myReq.theNextFunctionIndex = 0;

  // set the callback: if empty then this is a sync call for a single function
  myReq.theCallback = theCallback;

  // execute the function and return the response
  auto myResp = theClient->RunLambda(myReq, theDry);
  return std::make_unique<edge::LambdaResponse>(std::move(myResp));
}

std::unique_ptr<edge::LambdaResponse>
Client::functionChain(const std::string& aInput) {
  assert(theLambda.empty());
  assert(theCallback.empty());
  if (theChain.get() == nullptr) {
    throw std::runtime_error("uninitialized function chain");
  }

  std::string                           myInput = aInput;
  std::string                           myDataIn;
  std::unique_ptr<edge::LambdaResponse> myResp(nullptr);
  unsigned int                          myHops  = 0;
  unsigned int                          myPtime = 0;
  for (const auto& myFunction : theChain->functions()) {
    // create a request and fill it with the states needed by the function
    edge::LambdaRequest myReq(myFunction, myInput, myDataIn);
    for (const auto& myState : theChain->states(myFunction)) {
      const auto it = theStates.find(myState);
      assert(it != theStates.end());
      myReq.states().emplace(myState, edge::State(it->second));
    }

    // run the lambda function
    myResp = std::make_unique<edge::LambdaResponse>(
        theClient->RunLambda(myReq, theDry));

    // return immediately upon failure
    assert(myResp.get() != nullptr);
    VLOG(2) << *myResp;
    if (myResp->theRetCode != "OK") {
      break;
    }

    // use the return value to fill the next input
    myInput  = myResp->theOutput;
    myDataIn = myResp->theDataOut;

    // sum the hops and processing time
    myHops += myResp->theHops;
    myPtime += myResp->theProcessingTime;
  }

  // remove the states (if any) from the return to the caller
  myResp->states().clear();

  // if the number of functions is greater than 2, remove some
  // fields that are not meaningful
  if (theChain->functions().size() > 1) {
    myResp->theResponder.clear();
    myResp->theLoad1          = 0;
    myResp->theLoad10         = 0;
    myResp->theLoad30         = 0;
    myResp->theHops           = myHops;
    myResp->theProcessingTime = myPtime;
  }

  return myResp;
}

void Client::stop() {
  std::unique_lock<std::mutex> myLock(theMutex);
  if (theNotStartedFlag) {
    return;
  }
  theSendRequestQueue.close();
  theStopFlag = true;
  theSleepCondition.notify_one();
  theExitCondition.wait(myLock, [this]() { return theFinishedFlag; });
}

void Client::sendRequest(const size_t aSize) {
  auto mySize = aSize - 12;
  if (aSize < 12) {
    LOG_FIRST_N(WARNING, 1)
        << "requested input size " << aSize
        << " but cannot be smaller than 12 [will not be repeated]";
    mySize = 0;
  }

  std::string myContent;
  {
    const std::lock_guard<std::mutex> myLock(theMutex);
    myContent =
        theContent.empty() ?
            (std::string("{\"input\":\"") + std::string(mySize, 'A') + "\"}") :
            theContent;
  }

  // execute the main loop depending on the operating mode
  theLambdaChrono.start();
  std::unique_ptr<edge::LambdaResponse> myResp(nullptr);
  if (theLambda.empty() and theCallback.empty()) {
    myResp = functionChain(myContent);
  } else {
    myResp = singleExecution(myContent);
  }
  assert(myResp.get() != nullptr);
  if (not myResp->theAsynchronous) {
    recordStat(*myResp);
  }

  // wait until the stat has been recorded
  theSendRequestQueue.pop();
}

void Client::recordStat(const edge::LambdaResponse& aResponse) {
  // measure the application latency
  const auto myElapsed = theLambdaChrono.stop();

  // name to be used for logging and statistics
  const auto myName = theLambda.empty() ? theChain->name() : theLambda;

  if (aResponse.theRetCode != "OK") {
    VLOG(1) << "invalid response to " << myName << ": " << aResponse.theRetCode;

    // do not update the output and internal statistics in case of failure
    return;
  } else {
    VLOG(1) << myName << ", took " << (myElapsed * 1e3 + 0.5) << " ms, return "
            << aResponse;
  }

  if (theDry) {
    theSaver(aResponse.processingTimeSeconds(),
             aResponse.theLoad1,
             aResponse.theResponder,
             myName,
             aResponse.theHops);
  } else {
    theSaver(myElapsed,
             aResponse.theLoad1,
             aResponse.theResponder,
             myName,
             aResponse.theHops);
  }

  theLatencyStat(myElapsed);
  theProcessingStat(aResponse.processingTimeSeconds());

  // stat recorded, sendRequest() may proceed
  theSendRequestQueue.push(0);
}

void Client::sleep(const double aTime) {
  std::unique_lock<std::mutex> myLock(theMutex);
  theSleepCondition.wait_for(
      myLock,
      std::chrono::microseconds(static_cast<int64_t>(aTime * 1e6)),
      [this]() { return theStopFlag; });
}

void Client::setContent(const std::string& aContent) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  theContent = aContent;
}

void Client::setChain(const edge::model::Chain&            aChain,
                      const std::map<std::string, size_t>& aStateSizes) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  if (not theLambda.empty()) {
    throw std::runtime_error("cannot set chain info on a client operating in "
                             "single function execution mode");
  }

  LOG_IF(WARNING, theChain.get() != nullptr) << "overwriting an existing chain";

  // save the chain
  theChain = std::make_unique<edge::model::Chain>(aChain);

  // create the states without including free states (= those no functions
  // depend upon)
  theStates.clear();
  for (const auto& myState : aChain.allStates(false)) {
    const auto it = aStateSizes.find(myState);
    if (it == aStateSizes.end()) {
      throw std::runtime_error("the size of the state is missing: " + myState);
    }
    theStates[myState] = std::string(it->second, 'A');
  }
}

void Client::setCallback(const std::string& aCallback) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  if (aCallback.empty()) {
    throw std::runtime_error("cannot set an empty callback endpoint");
  }
  if (theCallback.empty()) {
    LOG(INFO) << "setting the callback end-point to " << aCallback;
  } else {
    LOG(WARNING) << "updating the callback end-point: " << theCallback << " -> "
                 << aCallback;
  }
  theCallback = aCallback;
}

void Client::setSizeDist(const size_t aSizeMin, const size_t aSizeMax) {
  LOG_IF(WARNING, theSizeDist != nullptr)
      << "changing the lambda request size r.v. parameters";
  theSizeDist.reset(new support::UniformIntRv<size_t>(
      aSizeMin, aSizeMax, theSeedUser, theSeedInc, 0));
  theSizes.clear(); // just in case it was not empty previously
}

void Client::setSizeDist(const std::vector<size_t>& aSizes) {
  if (aSizes.empty()) {
    throw std::runtime_error("Cannot draw sizes from an empty set");
  }
  LOG_IF(WARNING, theSizeDist != nullptr)
      << "changing the lambda request size r.v. parameters";
  theSizeDist.reset(new support::UniformIntRv<size_t>(
      0, aSizes.size() - 1, theSeedUser, theSeedInc, 0));
  theSizes = aSizes;
}

size_t Client::nextSize() {
  if (not theSizeDist) {
    throw std::runtime_error("The lambda request size r.v. is uninitialized");
  }
  const auto myValue = (*theSizeDist)();
  return theSizes.empty() ? myValue : theSizes.at(myValue);
}

} // namespace simulation
} // namespace uiiit
