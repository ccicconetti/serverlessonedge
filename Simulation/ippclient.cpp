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

#include "ippclient.h"

#include <cassert>
#include <sstream>
#include <thread>

#include <glog/logging.h>

namespace uiiit {
namespace simulation {

IppClient::IppClient(const double                 aTimeOn,
                     const double                 aTimeOff,
                     const double                 aShortSleepMin,
                     const double                 aShortSleepMax,
                     const size_t                 aSeedUser,
                     const size_t                 aSeedInc,
                     const size_t                 aNumRequests,
                     const std::set<std::string>& aServers,
                     const support::Conf&         aClientConf,
                     const std::string&           aLambda,
                     const support::Saver&        aSaver,
                     const bool                   aDry)
    : Client(aSeedUser,
             aSeedInc,
             aNumRequests,
             aServers,
             aClientConf,
             aLambda,
             aSaver,
             aDry)
    , theTimeOn(aTimeOn)
    , theTimeOff(aTimeOff)
    , theShortSleepMin(aShortSleepMin)
    , theShortSleepMax(aShortSleepMax)
    , theOnDist(1.0 / aTimeOn, theSeedUser, theSeedInc, 1)
    , theOffDist(1.0 / aTimeOff, theSeedUser, theSeedInc, 2)
    , theSleepDist(aShortSleepMin, aShortSleepMax, theSeedUser, theSeedInc, 3)
    , theBatch()
    , theState(State::On) // may be changed in the ctor body
    , theNextStateChange(0)
    , theOnChrono(false) {
  if (aShortSleepMin > aShortSleepMax or aShortSleepMax == 0) {
    throw std::runtime_error("Invalid min-sleep max-sleep choice");
  }
  const auto myRandOnTime  = theOnDist();
  const auto myRandOffTime = theOffDist();
  const auto myInitial     = support::UniformRv(
      0, myRandOnTime + myRandOffTime, theSeedUser, theSeedInc, 4)();
  if (myInitial < myRandOnTime) {
    theState           = State::On;
    theNextStateChange = myRandOnTime - myInitial;
  } else {
    theState           = State::Off;
    theNextStateChange = myRandOnTime + myRandOffTime - myInitial;
  }
}

IppClient::IppClient(const double                 aTimeOn,
                     const double                 aTimeOff,
                     const double                 aShortSleepMin,
                     const double                 aShortSleepMax,
                     const size_t                 aSizeMin,
                     const size_t                 aSizeMax,
                     const size_t                 aSeedUser,
                     const size_t                 aSeedInc,
                     const size_t                 aNumRequests,
                     const std::set<std::string>& aServers,
                     const support::Conf&         aClientConf,
                     const std::string&           aLambda,
                     const support::Saver&        aSaver,
                     const bool                   aDry)
    : IppClient(aTimeOn,
                aTimeOff,
                aShortSleepMin,
                aShortSleepMax,
                aSeedUser,
                aSeedInc,
                aNumRequests,
                aServers,
                aClientConf,
                aLambda,
                aSaver,
                aDry) {
  setSizeDist(aSizeMin, aSizeMax);
}

IppClient::IppClient(const double                 aTimeOn,
                     const double                 aTimeOff,
                     const double                 aShortSleepMin,
                     const double                 aShortSleepMax,
                     const std::vector<size_t>&   aSizes,
                     const size_t                 aSeedUser,
                     const size_t                 aSeedInc,
                     const size_t                 aNumRequests,
                     const std::set<std::string>& aServers,
                     const support::Conf&         aClientConf,
                     const std::string&           aLambda,
                     const support::Saver&        aSaver,
                     const bool                   aDry)
    : IppClient(aTimeOn,
                aTimeOff,
                aShortSleepMin,
                aShortSleepMax,
                aSeedUser,
                aSeedInc,
                aNumRequests,
                aServers,
                aClientConf,
                aLambda,
                aSaver,
                aDry) {
  setSizeDist(aSizes);
}

size_t IppClient::loop() {
  if (theState == State::Off) {
    //
    // OFF period
    //
    sleep(theNextStateChange);
    theState           = State::On;
    theNextStateChange = theOnDist();

    return 0;
  }
  //
  // ON period
  //

  if (not theOnChrono) {
    assert(theBatch.empty());

    // select the times at which the lambda requests should ideally be sent
    double myRelativeTime = 0;
    while (myRelativeTime < theNextStateChange) {
      theBatch.push(myRelativeTime);
      myRelativeTime += theSleepDist();
    }
    assert(not theBatch.empty());

    // start the chronometer for the current ON period
    theOnChrono.start();
  }

  // create the request and send it out
  theBatch.pop();
  sendRequest(nextSize());

  // if the batch of requests in the ON period is finished we can start a new
  // OFF period; also, if the next request would overflow the ON period, then
  // drop all the other requests in the batch and start a forced OFF period
  // immediately

  if (theBatch.empty() or theOnChrono.time() >= theNextStateChange) {
    // drop any pending request (sorry)
    LOG_IF(INFO, not theBatch.empty())
        << "dropping " << theBatch.size() << " lambda request"
        << (theBatch.size() == 1 ? "" : "s");
    std::queue<double> myEmptyQueue;
    theBatch.swap(myEmptyQueue);

    // schedule an immediate sleep period
    theState           = State::Off;
    theNextStateChange = theOffDist();
    theOnChrono.stop();

  } else {
    // wait until the next scheduled request, or do not wait at all if it is
    // already past due time
    assert(not theBatch.empty());
    const auto mySleepTime = theBatch.front() - theOnChrono.time();
    if (mySleepTime > 0) {
      sleep(mySleepTime);
    }
  }

  return 1;
}

size_t IppClient::simulate(const double aDuration) {
  assert(aDuration > 0);

  double myClock = 0; // virtual clock
  size_t ret     = 0; // number of requests generated

  // copy the actual state into local variables
  auto                   myState           = theState;
  auto                   myNextStateChange = theNextStateChange;
  support::ExponentialRv myOnDist(1.0 / theTimeOn, theSeedUser, theSeedInc, 1);
  support::ExponentialRv myOffDist(
      1.0 / theTimeOff, theSeedUser, theSeedInc, 2);
  support::UniformRv mySleepDist(
      theShortSleepMin, theShortSleepMax, theSeedUser, theSeedInc, 3);

  // discard one value of the on/off distributions
  myOnDist();
  myOffDist();

  std::stringstream myStream;
  while (myClock < aDuration) {
    if (myState == State::Off) {
      myClock += myNextStateChange;
      myState           = State::On;
      myNextStateChange = myOnDist();

    } else {
      assert(myState == State::On);
      const auto myOnFinishTime =
          std::min(aDuration, myClock + myNextStateChange);
      while (myClock < myOnFinishTime) {
        myStream << myClock << '\n';
        ret++; // lambda request sent
        myClock += mySleepDist();
      }
      myState           = State::Off;
      myNextStateChange = myOffDist();
    }
  }
  VLOG(2) << "simulated start times of lambda requests:\n" << myStream.str();

  return ret;
}

} // namespace simulation
} // namespace uiiit
