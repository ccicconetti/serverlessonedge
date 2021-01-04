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

#include "StateSim/scenario.h"

namespace uiiit {
namespace statesim {

Scenario::Scenario(const Conf& aConf)
    : theRng(aConf.theSeed)
    , theAffinities(makeAffinities(
          aConf.theFuncWeights, aConf.theAffinityWeights, theRng))
    , theNetwork(aConf.theNodesPath, aConf.theLinksPath, aConf.theEdgesPath)
    , theJobs(loadJobs(aConf.theTasksPath,
                       aConf.theOpsFactor,
                       aConf.theMemFactor,
                       aConf.theFuncWeights,
                       aConf.theSeed,
                       aConf.theStatefulOnly)) {
  // noop
}

std::map<std::string, Affinity>
Scenario::makeAffinities(const std::map<std::string, double> aFuncWeights,
                         const std::map<Affinity, double>&   aAffinities,
                         std::default_random_engine&         aRng) {
  std::map<std::string, Affinity> ret;

  double mySum = 0;
  for (const auto& elem : aAffinities) {
    mySum += elem.second;
  }
  std::uniform_real_distribution<double> myRv(0.0, mySum);
  for (const auto& elem : aFuncWeights) {
    const auto myRandomValue = myRv(aRng);
    auto       myCur         = 0.0;
    auto       it            = aAffinities.begin();
    for (; myRandomValue < myCur and it != aAffinities.end(); ++it) {
      myCur += it->second;
    }
    assert(it != aAffinities.end());
    ret.emplace(elem.first, it->first);
  }

  return ret;
}

} // namespace statesim
} // namespace uiiit
