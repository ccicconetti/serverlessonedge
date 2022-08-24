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

#include "LambdaMuSim/appperiods.h"

#include "Support/tostring.h"

#include <algorithm>
#include <stdexcept>

#include <glog/logging.h>

namespace uiiit {
namespace lambdamusim {

AppPeriods::AppPeriods(const dataset::TimestampDataset& aDataset,
                       const dataset::CostModel&        aCostModel,
                       const std::size_t                aMinPeriods)
    : thePeriods() {
  if (aMinPeriods == 0) {
    throw std::runtime_error("Invalid number of min periods, must be > 0");
  }

  // compute the mu/lambda periods according to the cost model
  auto myCosts = dataset::cost(aDataset, aCostModel, true);

  // save periods of functions with at least the given number of
  // invocations; trim the last period if there is an odd number of them
  for (auto& myCost : myCosts) {
    if (myCost.second.theBestNextPeriods.size() % 2 == 1) {
      // remove last period to make their number even
      myCost.second.theBestNextPeriods.pop_back();
    }
    if (myCost.second.theBestNextPeriods.size() < aMinPeriods) {
      // skip this app
      continue;
    }
    thePeriods.emplace_back(Periods());
    myCost.second.theBestNextPeriods.swap(thePeriods.back());
  }

  std::sort(thePeriods.begin(), thePeriods.end());

  VLOG(2) << aDataset.size() << " apps in dataset, " << thePeriods.size()
          << " filtered (>= " << aMinPeriods << " periods)";

  if (thePeriods.empty()) {
    throw std::runtime_error("Empty app traces");
  }
}

} // namespace lambdamusim
} // namespace uiiit
