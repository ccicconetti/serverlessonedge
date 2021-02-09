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

#include "utilestimator.h"

#include "Support/random.h"

#include <glog/logging.h>

#include <cassert>
#include <cmath>
#include <limits>

namespace uiiit {
namespace edge {

////////////////////////////////////////////////////////////////////////////////
// class UtilEstimator::LambdaDescriptor

UtilEstimator::LambdaDescriptor::LambdaDescriptor(
    UtilEstimator& aParent, const std::string& aDestination)
    : theParent(aParent)
    , theDestination(aDestination)
    , theEstimators() {
}

float UtilEstimator::LambdaDescriptor::ptime(const size_t aInputSize) {
  // find the last available load: if none then it is assumed to be 0
  auto jt = theParent.theComputers.find(theDestination);
  assert(jt != theParent.theComputers.end());
  if (jt == theParent.theComputers.end()) {
    LOG(WARNING) << "cannot estimate processing time of current lambda because "
                 << "its destination " << theDestination << " has disappeared";
    return 0.0f;
  }
  assert(jt->second != nullptr);
  unsigned int myLoad;
  double       myDeltaT;
  std::tie(myLoad, myDeltaT) = jt->second->lastLoad();

  // find the estimate for the given size: if not found then return 0
  const auto it = theEstimators.find(aInputSize);
  if (it != theEstimators.end()) {
    const auto myPtime = it->second->extrapolate(myLoad);
    VLOG(2) << "destination " << theDestination << ", last load " << myLoad
            << ", deltaT " << myDeltaT * 1e3 << " ms"
            << ", ptime " << myPtime * 1e3 << " ms";
    return myPtime;
  }

  return 0.0f;
}

void UtilEstimator::LambdaDescriptor::add(const size_t       aInputSize,
                                          const float        aPtime,
                                          const unsigned int aLoad) {
  // add/find an estimator for the current input size
  auto it = theEstimators.emplace(aInputSize, nullptr);
  if (it.second) {
    VLOG(2) << "creating load/ptime histogram for new input size: "
            << aInputSize;
    assert(not it.first->second);
    it.first->second.reset(
        new support::LinearEstimator(theParent.theWindowSize, -1.0));
  }
  assert(it.first != theEstimators.end());
  assert(it.first->second);

  // add current measurement: processing time as a function of the load
  LOG_IF(WARNING, aLoad > 100) << "overflowing load value: " << aLoad;
  it.first->second->add(aLoad, aPtime);
}

////////////////////////////////////////////////////////////////////////////////
// class UtilEstimator::ComputerDescriptor

UtilEstimator::ComputerDescriptor::ComputerDescriptor(const double aLoadTimeout)
    : theLambdas()
    , theLoadTimeout(aLoadTimeout)
    , theLastMeas(0)
    , theLastLoad1(0)
    , theChrono(true) {
}

std::pair<unsigned int, double> UtilEstimator::ComputerDescriptor::lastLoad() {
  const auto myNow = theChrono.time();

  // if the last load information is too old, then we cannot know the current
  // state of server: we reset the load to 0 to force a refresh of the state
  assert(myNow > theLastMeas);
  if (myNow - theLastMeas >= theLoadTimeout) {
    theLastLoad1 = 0;
    theLastMeas  = myNow; // it's as if we had received a load1 == 0
  }

  const auto myDeltaT = myNow - theLastMeas;
  assert(myDeltaT >= 0);

  return std::make_pair(theLastLoad1, myDeltaT);
}

void UtilEstimator::ComputerDescriptor::add(const unsigned int aLoad1) {
  LOG_IF(WARNING, aLoad1 > 100) << "overflowing load1 value: " << aLoad1;

  // update the last time we have received a measurement
  theLastMeas  = theChrono.time();
  theLastLoad1 = std::min(99u, aLoad1);
}

////////////////////////////////////////////////////////////////////////////////
// class UtilEstimator

UtilEstimator::UtilEstimator(const double aLoadTimeout,
                             const size_t aUtilWindowSize)
    : theLoadTimeout(aLoadTimeout)
    , theWindowSize(aUtilWindowSize)
    , theComputers()
    , theTable([this](const std::string&, const std::string& aDestination) {
      return std::make_unique<LambdaDescriptor>(*this, aDestination);
    }) {
}

UtilEstimator::~UtilEstimator() {
  // nothing
}

std::tuple<std::string, float, float>
UtilEstimator::best(const std::string&                  aLambda,
                    const size_t                        aInputSize,
                    const std::map<std::string, float>& aRtts) {
  const auto myPtimes =
      theTable.all(aLambda, [aInputSize](LambdaDescriptor& aLambdaDescriptor) {
        return aLambdaDescriptor.ptime(aInputSize);
      });

  // the tables pTimes and aRtts should have the same keys, hence they can be
  // navigated one step at a time together
  assert(not myPtimes.empty());
  assert(aRtts.size() == myPtimes.size());

  std::tuple<std::string, float, float> ret;
  auto myMinTime = std::numeric_limits<float>::max();
  auto it        = aRtts.begin();
  auto jt        = myPtimes.begin();
  for (; it != aRtts.end() and jt != myPtimes.end(); ++it, ++jt) {
    assert(it->first == jt->first);
    const auto myCurTime = it->second + jt->second;
    VLOG(2) << aLambda << ", in_size " << aInputSize << ", destination "
            << it->first << ", rtt_est " << it->second << " s, ptime_est "
            << jt->second << " s, tot_est " << myCurTime << " s, candidate "
            << myMinTime << " s";
    assert(not std::isnan(it->second));
    assert(not std::isnan(jt->second));
    if (myCurTime < myMinTime) {
      std::get<BT_DEST>(ret)  = it->first;
      std::get<BT_RTT>(ret)   = it->second;
      std::get<BT_PTIME>(ret) = jt->second;
      myMinTime               = myCurTime;
    }
  }

  assert(it == aRtts.end());
  assert(jt == myPtimes.end());
  assert(myMinTime < std::numeric_limits<float>::max());

  return ret;
}

std::pair<std::string, float>
UtilEstimator::smallestPtime(const std::string& aLambda,
                             const size_t       aInputSize) {
  const auto ret =
      theTable.best(aLambda, [aInputSize](LambdaDescriptor& aDescriptor) {
        return -aDescriptor.ptime(aInputSize);
      });
  return std::make_pair(ret.first, -ret.second);
}

void UtilEstimator::add(const std::string& aLambda,
                        const std::string& aDestination,
                        const size_t       aInputSize,
                        const float        aPtime,
                        const unsigned int aLoad1,
                        const unsigned int aLoad10) {
  std::ignore = aLoad10;
  theTable.find(aLambda, aDestination).add(aInputSize, aPtime, aLoad1);
  const auto it = theComputers.find(aDestination);
  assert(it != theComputers.end());
  if (it != theComputers.end()) {
    assert(it->second != nullptr);
    it->second->add(aLoad1);
  }
}

bool UtilEstimator::add(const std::string& aLambda,
                        const std::string& aDestination) {

  auto it = theComputers.find(aDestination);
  if (it == theComputers.end()) {
    bool myAdded;
    std::tie(it, myAdded) = theComputers.emplace(
        aDestination, std::make_unique<ComputerDescriptor>(theLoadTimeout));
    assert(myAdded);
  }
  assert(it != theComputers.end());
  assert(it->second != nullptr);
  it->second->theLambdas.insert(aLambda);
  return theTable.add(aLambda, aDestination);
}

bool UtilEstimator::remove(const std::string& aLambda,
                           const std::string& aDestination) {
  auto it = theComputers.find(aDestination);
  assert(it != theComputers.end());
  if (it != theComputers.end()) {
    assert(it->second != nullptr);
    it->second->theLambdas.erase(aLambda);
    if (it->second->theLambdas.empty()) {
      theComputers.erase(it);
    }
  }
  return theTable.remove(aLambda, aDestination);
}

} // namespace edge
} // namespace uiiit
