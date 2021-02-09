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
    , theLambdaChrono(false)
    , theClient(edge::EdgeClientFactory::make(aServers, aClientConf))
    , theNumRequests(aNumRequests)
    , theLambda(aLambda)
    , theSaver(aSaver)
    , theLatencyStat()
    , theProcessingStat()
    , theDry(aDry)
    , theSizeDist()
    , theSizes()
    , theContent() {
}

Client::~Client() {
  stop();
}

void Client::operator()() {
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

void Client::stop() {
  std::unique_lock<std::mutex> myLock(theMutex);
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

  edge::LambdaRequest myReq(theLambda, myContent);

  theLambdaChrono.start();
  const auto myResp    = theClient->RunLambda(myReq, theDry);
  const auto myElapsed = theLambdaChrono.stop();

  if (myResp.theRetCode != "OK") {
    // do not update the output and internal statistics in case of failure
    VLOG(1) << "invalid response to " << theLambda << ": " << myResp.theRetCode;
    return;
  }

  VLOG(1) << theLambda << ", took " << (myElapsed * 1e3 + 0.5) << " ms, return "
          << myResp;

  if (theDry) {
    theSaver(myResp.processingTimeSeconds(),
             myResp.theLoad1,
             myResp.theResponder,
             theLambda,
             myResp.theHops);
  } else {
    theSaver(myElapsed,
             myResp.theLoad1,
             myResp.theResponder,
             theLambda,
             myResp.theHops);
  }

  theLatencyStat(myElapsed);
  theProcessingStat(myResp.processingTimeSeconds());
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
