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

#pragma once

#include "Edge/Entries/entry.h"
#include "Support/chrono.h"

#include <cmath>
#include <list>
#include <string>

namespace uiiit {
namespace edge {
namespace entries {

/**
 * Proportional Fairness Entry:
 * If the are multiple options available, the chosen entry must be the one with
 * maximum prioritization coefficient (computed in the functional operator each
 * time a destination for a given LambdaRequest must be scheduled)
 *
 * Prioritization coefficients PC are computed as: PC = T^alpha/R^beta where:
 * T = inverse of the last latency experienced serving a lambda request using
 * the destination
 * R = ratio between theLambdaServedCount and the difference between the
 * timestamp at scheduling time and theTimestamp associated with the
 * destination (the latter can be the timestamp in which the destination has
 * joined the system or the timestamp related to the last successful service of
 * a LambdaRequest given by the destination)
 * alpha,beta = are parametere obtained by CLI to tune the fairness of the
 * scheduler:
 *    - alpha = 0, beta = 1 => Round Robin
 *    - alpha = 1, beta = 0 => max unfairness, max throughput
 *    - alpha ~= 1, beta ~= 1 => 3G scheduling algorithm
 */
class EntryProportionalFairness final : public Entry
{
 public:
  explicit EntryProportionalFairness(double aAlpha, double aBeta);

 private:
  std::string operator()() override;

  void updateWeight(const std::string& aDest,
                    const float        aOldWeight,
                    const float        aNewWeight) override;
  void updateAddDest(const std::string& aDest, const float aWeight) override;
  void updateDelDest(const std::string& aDest, const float aWeight) override;

  float computeWeight(float  experiencedLatency,
                      int    theLambdaServedCount,
                      double theDestinationTimestamp,
                      double curTimestamp);

 private:
  support::Chrono theChrono;
  const double    theAlpha;
  const double    theBeta;
};

} // namespace entries
} // namespace edge
} // namespace uiiit