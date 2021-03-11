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

#include "Edge/Entries/entryproportionalfairness.h"

#include "Edge/forwardingtableexceptions.h"

#include <algorithm>
#include <cassert>

namespace uiiit {
namespace edge {
namespace entries {

EntryProportionalFairness::EntryProportionalFairness(double aAlpha,
                                                     double aBeta)
    : Entry()
    , theChrono(true)
    , theAlpha(aAlpha)
    , theBeta(aBeta) {
}

/**
 * The Entry functional operator must compute all the prioritarization
 * coefficients and return the maximum one.
 */
std::string EntryProportionalFairness::operator()() {
  if (theDestinations.empty()) {
    throw NoDestinations();
  }

  std::string ret;
  float       myMaxWeight;
  const auto  myNow = theChrono.time();

  for (auto it = theDestinations.begin(); it != theDestinations.end(); ++it) {
    auto destinationStatsIt = thePFstats.find((*it).theDestination);
    assert(destinationStatsIt != thePFstats.end());

    const auto myCurWeight = computeWeight((*it).theWeight,
                                           (*destinationStatsIt).second.first,
                                           (*destinationStatsIt).second.second,
                                           myNow);
    if (ret.empty() or myCurWeight > myMaxWeight) {
      myMaxWeight = myCurWeight;
      ret         = (*it).theDestination;
    }
  }

  return ret;
}

/**
 * experiencedLatency is the latency experienced and stored in theWeight field
 * by the LocalOptimizer during the processSuccess or 1  if the entry is "fresh"
 * (just inserted by the controller)
 */
float EntryProportionalFairness::computeWeight(float  experiencedLatency,
                                               int    theLambdaServedCount,
                                               double theDestinationTimestamp,
                                               double curTimestamp) {
  return pow((1 / experiencedLatency), theAlpha) /
         pow((theLambdaServedCount / (curTimestamp - theDestinationTimestamp)),
             theBeta);
}

/**
 * this function only updates thePFstats (theLambdaServedCount and the
 * Timestamp) of the destination which has served the LambdaRequest
 */
void EntryProportionalFairness::updateWeight(
    const std::string&           aDest,
    [[maybe_unused]] const float aOldWeight,
    [[maybe_unused]] const float aNewWeight) {
  const auto myNow = theChrono.time();
  auto       it    = thePFstats.find(aDest);
  assert(it != thePFstats.end());
  it->second = {it->second.first + 1, myNow};
  printPFstats();
}

/**
 * this function only updates thePFstats data structure inserting a new
 * destination as key and a pair in which theLambdaServedCount is set to 1
 * otherwise a "division by 0" exception is thrown during the computation of its
 * weight in the functional operator (invoked by the EdgeRouter.destination).
 */
void EntryProportionalFairness::updateAddDest(
    const std::string& aDest, [[maybe_unused]] const float aWeight) {
  thePFstats.insert({aDest, std::make_pair(1, theChrono.time())});
  printPFstats();
}

/**
 * this function only updates thePFstats data structure deleting the detination
 * removed in theDestination data structure.
 */
void EntryProportionalFairness::updateDelDest(
    const std::string& aDest, [[maybe_unused]] const float aWeight) {
  auto it = thePFstats.find(aDest);
  assert(it != thePFstats.end());
  thePFstats.erase(it);
  printPFstats();
}

} // namespace entries
} // namespace edge
} // namespace uiiit
