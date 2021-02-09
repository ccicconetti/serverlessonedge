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

#include "rttestimator.h"

#include "Edge/forwardingtableexceptions.h"

#include <glog/logging.h>

namespace uiiit {
namespace edge {

////////////////////////////////////////////////////////////////////////////////
// class RttEstimator::Descriptor

RttEstimator::Descriptor::Descriptor(const size_t aWindowSize,
                                     const double aStalePeriod)
    : theEstimator(aWindowSize, aStalePeriod) {
}

float RttEstimator::Descriptor::rtt(const size_t aInputSize) {
  // note: it may happen that all the values are purged. This happens if a given
  // destination has not been used for the stale period, e.g., because a better
  // destination was consistently found. After purge, the window will be empty,
  // which means that the slope/intercept values will be 0. This is done on
  // purpose, since it forces a destination with stale measurements to have a
  // higher change to be picked so as to act as a sort of periodic probing
  // mechanism
  return std::max(0.0f, theEstimator.extrapolate(aInputSize));
}

void RttEstimator::Descriptor::add(const size_t aInputSize, const float aRtt) {
  theEstimator.add(aInputSize, aRtt);
}

////////////////////////////////////////////////////////////////////////////////
// class RttEstimator

RttEstimator::RttEstimator(const size_t aWindowSize, const double aStalePeriod)
    : theTable(
          [aWindowSize, aStalePeriod](const std::string&, const std::string&) {
            return std::make_unique<Descriptor>(aWindowSize, aStalePeriod);
          }) {
}

RttEstimator::~RttEstimator() {
}

float RttEstimator::rtt(const std::string& aLambda,
                        const std::string& aDestination,
                        const size_t       aInputSize) {
  try {
    return theTable.find(aLambda, aDestination).rtt(aInputSize);
  } catch (const InvalidDestination&) {
    // ignore
  }
  return 0.0f;
}

std::map<std::string, float> RttEstimator::rtts(const std::string& aLambda,
                                                const size_t       aInputSize) {
  return theTable.all(aLambda, [aInputSize](Descriptor& aDescriptor) {
    return aDescriptor.rtt(aInputSize);
  });
}

std::pair<std::string, float>
RttEstimator::shortestRtt(const std::string& aLambda, const size_t aInputSize) {
  const auto ret =
      theTable.best(aLambda, [aInputSize](Descriptor& aDescriptor) {
        return -aDescriptor.rtt(aInputSize);
      });
  return std::make_pair(ret.first, -ret.second);
}

void RttEstimator::add(const std::string& aLambda,
                       const std::string& aDestination,
                       const size_t       aInputSize,
                       const float        aRtt) {
  theTable.find(aLambda, aDestination).add(aInputSize, aRtt);
}

bool RttEstimator::add(const std::string& aLambda,
                       const std::string& aDestination) {
  return theTable.add(aLambda, aDestination);
}

bool RttEstimator::remove(const std::string& aLambda,
                          const std::string& aDestination) {
  return theTable.remove(aLambda, aDestination);
}

} // namespace edge
} // namespace uiiit
