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

#include "LambdaMuSim/appmodel.h"

#include "Support/split.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace uiiit {
namespace lambdamusim {

std::unique_ptr<AppModel> makeAppModel(const size_t       aSeed,
                                       const std::string& aDescription) {
  const auto myTokens =
      support::split<std::vector<std::string>>(aDescription, ",");
  if (myTokens.empty()) {
    throw std::runtime_error("Invalid app model description: empty string");
  }
  if (myTokens[0] == "constant") {
    if (myTokens.size() != 4) {
      throw std::runtime_error("Invalid constant app model description: wrong "
                               "number of parameters, expected 4 but found " +
                               std::to_string(myTokens.size() - 1));
    }
    return std::make_unique<AppModelConstant>(
        AppModel::Params{std::stol(myTokens[1]),
                         std::stol(myTokens[2]),
                         std::stol(myTokens[3])});
  } else if (myTokens[0] == "classes") {
    if (myTokens.size() == 1) {
      throw std::runtime_error(
          "Invalid multi-class app model description: no class defined");
    }
    if (((myTokens.size() - 1) % 4) != 0) {
      throw std::runtime_error(
          "Invalid multi-class app model description: wrong "
          "number of parameters (" +
          std::to_string(myTokens.size() - 1) + ")");
    }
    std::map<double, AppModel::Params> myParams;
    auto                               myOffset = 1u;
    while (myOffset < myTokens.size()) {
      myParams.emplace(std::stod(myTokens[myOffset]),
                       AppModel::Params{std::stol(myTokens[myOffset + 1]),
                                        std::stol(myTokens[myOffset + 2]),
                                        std::stol(myTokens[myOffset + 3])});
      myOffset += 4;
    }
    assert(not myParams.empty());
    return std::make_unique<AppModelClasses>(aSeed, myParams);
  }
  throw std::runtime_error("Invalid app model type: " + myTokens[0]);
}

AppModelConstant::AppModelConstant(const Params& aParams)
    : AppModel()
    , theParams(aParams) {
  // noop
}

AppModel::Params AppModelConstant::operator()() {
  return theParams;
}

AppModelClasses::AppModelClasses(const size_t                    aSeed,
                                 const std::map<double, Params>& aParams)
    : AppModel()
    , theRv(0.0,
            std::accumulate(aParams.begin(),
                            aParams.end(),
                            0.0,
                            [](const double sum, const auto& elem) {
                              return sum + elem.first;
                            }),
            aSeed,
            0,
            0)
    , theParams(aParams) {
  if (aParams.empty()) {
    throw std::runtime_error("No application classes defined");
  }
}

AppModel::Params AppModelClasses::operator()() {
  const auto myRnd = theRv();
  auto       mySum = 0.0;
  auto       it    = theParams.cbegin();
  for (; it != theParams.cend(); ++it) {
    mySum += it->first;
    if (mySum >= myRnd) {
      break;
    }
  }
  if (it == theParams.cend()) {
    // I think this _might_ happen due to floating point rounding
    assert(theParams.crbegin() != theParams.crend());
    return theParams.crbegin()->second;
  }
  return it->second;
}

} // namespace lambdamusim
} // namespace uiiit
