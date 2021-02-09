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

#include "unifclient.h"

#include "Support/random.h"

#include <glog/logging.h>

#include <cassert>
#include <thread>

namespace uiiit {
namespace simulation {

UnifClient::UnifClient(const std::vector<size_t>&   aSizes,
                       const double                 aInterRequestTime,
                       const Distribution           aDistribution,
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
    , theSleepDist(aDistribution == Distribution::CONSTANT ?
                       RvPtr(new support::ConstantRv(aInterRequestTime)) :
                       aDistribution == Distribution::UNIFORM ?
                       RvPtr(new support::UniformRv(
                           0, 2 * aInterRequestTime, aSeedUser, aSeedInc, 1)) :
                       RvPtr(new support::ExponentialRv(
                           1.0 / aInterRequestTime, aSeedUser, aSeedInc, 1)))
    , theLastTime(0)
    , theChrono(false) {
  setSizeDist(aSizes);
}

size_t UnifClient::loop() {
  if (not theChrono) {
    theChrono.start();
    sleep((*theSleepDist)());
    theLastTime = theChrono.time();

    return 0;
  }

  const auto myNext = (*theSleepDist)();

  // create the request and send it out
  sendRequest(nextSize());

  // wait before next burst
  const auto mySleepTime = myNext - (theChrono.time() - theLastTime);
  if (mySleepTime > 0) {
    sleep(mySleepTime);
  } else {
    // log only lags greater than 1 second
    if (mySleepTime < -1.0) {
      LOG_FIRST_N(WARNING, 10)
          << "lagging behind by " << (-mySleepTime * 1e3) << " ms ["
          << google::COUNTER << "/10 instances shown]";
    }
  }

  theLastTime += myNext;

  return 1;
}

size_t UnifClient::simulate(const double aDuration) {
  throw std::runtime_error("Not yet implemented");
}

UnifClient::Distribution distributionFromString(const std::string& aType) {
  if (aType == "constant") {
    return UnifClient::Distribution::CONSTANT;
  } else if (aType == "uniform") {
    return UnifClient::Distribution::UNIFORM;
  } else if (aType == "poisson") {
    return UnifClient::Distribution::EXPONENTIAL;
  }
  throw std::runtime_error("Invalid distribution type: " + aType);
}

} // namespace simulation
} // namespace uiiit
