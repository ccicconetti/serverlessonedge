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

#include "poissonclient.h"

#include <glog/logging.h>

#include <cassert>
#include <thread>

namespace uiiit {
namespace simulation {

PoissonClient::PoissonClient(const double          aMeanPeriod,
                             const double          aMeanBurstSize,
                             const double          aShortSleepMin,
                             const double          aShortSleepMax,
                             const size_t          aSeedUser,
                             const size_t          aSeedInc,
                             const size_t          aNumRequests,
                     const std::set<std::string>& aServers,
                     const support::Conf&         aClientConf,
                             const std::string&    aLambda,
                             const support::Saver& aSaver,
                             const bool            aDry)
    : Client(aSeedUser,
             aSeedInc,
             aNumRequests,
             aServers,aClientConf,
             aLambda,
             aSaver,
             aDry)
    , thePeriodDist(1.0 / aMeanPeriod, aSeedUser, aSeedInc, 1)
    , theBurstSizeDist(aMeanBurstSize, aSeedUser, aSeedInc, 2)
    , theSleepDist(aShortSleepMin, aShortSleepMax, aSeedUser, aSeedInc, 3)
    , theCurrentBurst(0)
    , theCurrentPeriod(thePeriodDist() / 2)
    , thePeriodChrono(false) {
}

PoissonClient::PoissonClient(const double          aMeanPeriod,
                             const double          aMeanBurstSize,
                             const double          aShortSleepMin,
                             const double          aShortSleepMax,
                             const size_t          aSizeMin,
                             const size_t          aSizeMax,
                             const size_t          aSeedUser,
                             const size_t          aSeedInc,
                             const size_t          aNumRequests,
                     const std::set<std::string>& aServers,
                     const support::Conf&         aClientConf,
                             const std::string&    aLambda,
                             const support::Saver& aSaver,
                             const bool            aDry)
    : PoissonClient(aMeanPeriod,
                    aMeanBurstSize,
                    aShortSleepMin,
                    aShortSleepMax,
                    aSeedUser,
                    aSeedInc,
                    aNumRequests,
                    aServers,aClientConf,
                    aLambda,
                    aSaver,
                    aDry) {
  setSizeDist(aSizeMin, aSizeMax);
}

PoissonClient::PoissonClient(const double               aMeanPeriod,
                             const double               aMeanBurstSize,
                             const double               aShortSleepMin,
                             const double               aShortSleepMax,
                             const std::vector<size_t>& aSizes,
                             const size_t               aSeedUser,
                             const size_t               aSeedInc,
                             const size_t               aNumRequests,
                     const std::set<std::string>& aServers,
                     const support::Conf&         aClientConf,
                             const std::string&         aLambda,
                             const support::Saver&      aSaver,
                             const bool                 aDry)
    : PoissonClient(aMeanPeriod,
                    aMeanBurstSize,
                    aShortSleepMin,
                    aShortSleepMax,
                    aSeedUser,
                    aSeedInc,
                    aNumRequests,
                    aServers,aClientConf,
                    aLambda,
                    aSaver,
                    aDry) {
  setSizeDist(aSizes);
}

size_t PoissonClient::loop() {
  if (theCurrentBurst == 0) {
    // sleep until the next period, unless we are running late
    const auto myElapsed = thePeriodChrono.stop();

    const auto myRemaining = theCurrentPeriod - myElapsed;

    if (myRemaining > 0) {
      sleep(myRemaining);
    } else {
      LOG(WARNING) << "Running late by " << -myRemaining << " s";
    }

    // select next period duration and burst size
    theCurrentPeriod = thePeriodDist();
    theCurrentBurst  = theBurstSizeDist();
    thePeriodChrono.start();

    VLOG(2) << "Selected a burst of " << theCurrentBurst
            << " requests, next burst in " << theCurrentPeriod << " s";

    return 0;
  }

  assert(thePeriodChrono);

  // create the request and send it out
  sendRequest(nextSize());

  theCurrentBurst--;

  // wait before next burst
  if (theCurrentBurst > 0) {
    sleep(theSleepDist());
  }

  return 1;
}

size_t PoissonClient::simulate(const double aDuration) {
  throw std::runtime_error("Not yet implemented");
}

} // namespace simulation
} // namespace uiiit
