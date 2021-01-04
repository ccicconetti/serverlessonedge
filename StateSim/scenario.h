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

#pragma once

#include "StateSim/affinity.h"
#include "StateSim/job.h"
#include "StateSim/network.h"
#include "Support/macros.h"

#include <map>
#include <random>
#include <string>

namespace uiiit {
namespace statesim {

class Scenario
{
  NONCOPYABLE_NONMOVABLE(Scenario);

 public:
  struct Conf {
    //! File containing the info about nodes
    const std::string& theNodesPath;
    //! File containing the info about links
    const std::string theLinksPath;
    //! File containing the info about edges
    const std::string theEdgesPath;
    //! File containing the info about tasks
    const std::string theTasksPath;
    //! Multiplier for number of operations of tasks
    const double theOpsFactor;
    //! Multiplier for amount of memory/argument size of tasks
    const double theMemFactor;
    //! Weights to draw randomly assignment of lambda functions
    const std::map<std::string, double>& theFuncWeights;
    //! Seed to initialize random number generation
    const size_t theSeed;
    //! True if only stateful jobs are considered
    const bool theStatefulOnly;
    //! Weights to draw randomly affinity of lambda functions
    const std::map<Affinity, double> theAffinityWeights;
  };

  explicit Scenario(const Conf& aConf);

 private:
  static std::map<std::string, Affinity>
  makeAffinities(const std::map<std::string, double> aFuncWeights,
                 const std::map<Affinity, double>&   aAffinityWeights,
                 std::default_random_engine&         aRng);

 private:
  std::default_random_engine            theRng;
  const std::map<std::string, Affinity> theAffinities;
  Network                               theNetwork;
  std::vector<Job>                      theJobs;
};

} // namespace statesim
} // namespace uiiit
