/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2022 C. Cicconetti <https://ccicconetti.github.io/>

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

#include "LambdaMuSim/apppool.h"

#include "Support/random.h"
#include "Support/tostring.h"

#include <cassert>
#include <cstddef>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <glog/logging.h>

namespace uiiit {
namespace lambdamusim {

AppPool::AppPool(const std::vector<std::deque<double>>& aPeriods,
                 const std::size_t                      aNumApps,
                 const std::size_t                      aSeed)
    : thePeriods(aPeriods) {
  if (aNumApps == 0) {
    throw std::runtime_error("Invalid number of applications, must be > 0");
  }

  // create the pool of applications
  support::UniformIntRv<std::size_t> myRvPeriods(
      0, thePeriods.size() - 1, aSeed, 0, 0);

  for (std::size_t i = 0; i < aNumApps; i++) {
    const auto myPeriodId = myRvPeriods();
    auto       it         = thePeriods[myPeriodId].begin();
    assert(thePeriods[myPeriodId].size() % 2 == 0);
    const auto myOffset =
        2 * support::UniformIntRv<std::size_t>(
                0, (thePeriods[myPeriodId].size() / 2) - 1, aSeed, i, 0)();

    std::advance(it, myOffset);
    assert(it != thePeriods[myPeriodId].end());
    thePool.emplace_back(myPeriodId,
                         it,
                         thePeriods[myPeriodId].begin(),
                         thePeriods[myPeriodId].end());
    theDesc.emplace_back(i, *it);
  }

  theDesc.sort();

  for (auto it = std::next(theDesc.begin()); it != theDesc.end(); ++it) {
    assert(it->theRemaining >= std::prev(it)->theRemaining);
    it->theRemaining -= std::prev(it)->theRemaining;
  }

  for (const auto& myDesc : theDesc) {
    std::stringstream myStream;
    const auto&       myApp      = thePool[myDesc.theAppId];
    double            myDuration = 0;
    for (auto it = myApp.theCurrent; it != myApp.theEnd; ++it) {
      myStream << ' ' << *it;
      myDuration += *it;
    }
    for (auto it = myApp.theBegin; it != myApp.theCurrent; ++it) {
      myStream << ' ' << *it;
      myDuration += *it;
    }
    VLOG(2) << "#" << myDesc.theAppId << ", period id "
            << thePool[myDesc.theAppId].thePeriodId << ", remaining "
            << myDesc.theRemaining << ", duration " << (myDuration * 1e-6)
            << "M, periods (size " << thePeriods[myApp.thePeriodId].size()
            << " ) [" << myStream.str() << " ]";
  }
}

double AppPool::next() const {
  assert(not theDesc.empty());
  return theDesc.front().theRemaining;
}

void AppPool::advance(const double aTime) {
  assert(not theDesc.empty());
  assert(theDesc.front().theRemaining >= aTime);
  theDesc.front().theRemaining -= aTime;
}

std::pair<std::size_t, double> AppPool::advance() {
  assert(not theDesc.empty());

  auto mySelected = theDesc.front();
  theDesc.pop_front();

  // advance the app selected and re-push it into the differential list
  auto& myApp = thePool[mySelected.theAppId];
  std::advance(myApp.theCurrent, 1);
  if (myApp.theCurrent == myApp.theEnd) {
    // wrap around
    myApp.theCurrent = myApp.theBegin;
  }
  Desc myNewDesc(mySelected.theAppId,
                 mySelected.theRemaining + *myApp.theCurrent);
  auto myAdded = false;
  for (auto it = theDesc.begin(); it != theDesc.end(); ++it) {
    if (myNewDesc.theRemaining <= it->theRemaining) {
      it->theRemaining -= myNewDesc.theRemaining;
      theDesc.insert(it, myNewDesc);
      myAdded = true;
      break;
    } else {
      assert(myNewDesc.theRemaining > it->theRemaining);
      myNewDesc.theRemaining -= it->theRemaining;
    }
  }
  if (not myAdded) {
    theDesc.push_back(myNewDesc);
  }
  assert(theDesc.size() == thePool.size());

  return {mySelected.theAppId, mySelected.theRemaining};
}

} // namespace lambdamusim
} // namespace uiiit
