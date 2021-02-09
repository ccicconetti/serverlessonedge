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

#include "entry.h"

#include "Support/chrono.h"
#include "Support/stat.h"

#include <list>
#include <map>
#include <queue>
#include <string>

namespace uiiit {
namespace edge {
namespace entries {

/**
 * Keep an active set of destinations including only those whose weight is
 * smaller than or equal to twice the minimum. Among the active destinations,
 * select the one with lowest deficit counter. Whenever a destination is
 * selected, increment its deficit counter by the weight. This mechanism
 * achieves a sort of weighted round robin effect since destinations with lower
 * weights will be selected proportionally more than those with a higher weight.
 *
 * After a stale period expires for a destination outside of the active set, it
 * is added to active set irrespective of its weight for a single probe request.
 * The stale period id doubled every time a destination is moved away from the
 * active set and it is reset to the initial value if its weight eventually
 * matches the active set admission condition.
 */
class EntryRoundRobin final : public Entry
{
  struct CacheElement final {
    /**
     * Create a cache element with given weight and deficit, marked as "never
     * used before" and with an initial stale period duration equal to 1 s.
     */
    CacheElement(const float aWeight, const float aDeficit)
        : theWeight(aWeight)
        , theLastUpdated(-1.0)
        , theRemoved(false)
        , theDeficit(aDeficit)
        , theStalePeriod(initialStalePeriod())
        , theProbing(false) {
    }

    void resetStalePeriod() {
      theStalePeriod = initialStalePeriod();
    }

    void updateStalePeriod() {
      theStalePeriod =
          std::min(maximumStalePeriod(), backoffCoefficient() * theStalePeriod);
    }

    float  theWeight;
    double theLastUpdated; // in seconds, negative means never
    bool   theRemoved;
    float  theDeficit;
    double theStalePeriod; // in seconds
    bool   theProbing;
  };
  using Cache = std::map<std::string, CacheElement>;

  struct QueueElement final {
    Cache::iterator theElem;

    bool operator<(const QueueElement& aOther) const noexcept {
      return theElem->second.theDeficit > aOther.theElem->second.theDeficit;
    }
  };

 public:
  explicit EntryRoundRobin();

 private:
  std::string operator()() override;

  void updateWeight(const std::string& aDest,
                    const float        aOldWeight,
                    const float        aNewWeight) override;
  void updateAddDest(const std::string& aDest, const float aWeight) override;
  void updateDelDest(const std::string& aDest, const float aWeight) override;

  void  updateWeight(const std::string& aDest, const float aWeight);
  void  updateList();
  void  updateStat();
  void  updateActiveSet();
  float minDeficit();

  //! \return true if a weight is good enough.
  bool good(const double aWeight);

  void debugPrintActiveSet();

 private:
  // the active set consists of all the elements with similar weight
  // and all the elements that have not been used for too long
  Cache                             theCache;
  std::priority_queue<QueueElement> theQueue;
  support::Chrono                   theChrono;
  support::SummaryStat              theStat;

  // static configuration
  // clang-format off
  static constexpr double initialStalePeriod() { return 1.0 ; } 
  static constexpr double backoffCoefficient() { return 2.0 ; }
  static constexpr double maximumStalePeriod() { return 30.0; }
  // clang-format on
};

} // namespace entries
} // namespace edge
} // namespace uiiit
