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

#pragma once

#include "Support/macros.h"

#include <cinttypes>
#include <deque>
#include <list>
#include <string>
#include <vector>

namespace uiiit {
namespace lambdamusim {

class AppPool final
{
  NONCOPYABLE_NONMOVABLE(AppPool);

  using Periods = std::deque<double>;

  struct App {
    explicit App(std::size_t             aPeriodId,
                 Periods::const_iterator aCurrent,
                 Periods::const_iterator aBegin,
                 Periods::const_iterator aEnd)
        : thePeriodId(aPeriodId)
        , theCurrent(aCurrent)
        , theBegin(aBegin)
        , theEnd(aEnd) {
      // noop
    }
    const std::size_t             thePeriodId;
    Periods::const_iterator       theCurrent;
    const Periods::const_iterator theBegin;
    const Periods::const_iterator theEnd;
  };

  struct Desc {
    explicit Desc(const std::size_t aAppId, const double aRemaining)
        : theAppId(aAppId)
        , theRemaining(aRemaining) {
      // noop
    }
    bool operator<(const Desc& aOther) const noexcept {
      return theRemaining < aOther.theRemaining;
    }
    std::size_t theAppId;
    double      theRemaining;
  };

 public:
  /**
   * @brief Construct a new App Pool object.
   *
   * @param aPeriods The apps' periods.
   * @param aNumApps The number of applications in pool.
   * @param aSeed The RNG seed.
   */
  AppPool(const std::vector<std::deque<double>>& aPeriods,
          const std::size_t                      aNumApps,
          const std::size_t                      aSeed);

  //! @return the shortest remaining time.
  double next() const;

  /**
   * @brief Advance time until the next event, i.e., the transition of the app
   * with shortest remaining time.
   *
   * @return The index of the app advanced and the time passed.
   */
  std::pair<std::size_t, double> advance();

  //! Advance the clock by the given amount.
  void advance(const double aTime);

 private:
  const std::vector<std::deque<double>>& thePeriods;
  std::vector<App>                       thePool;
  std::list<Desc>                        theDesc;
};

} // namespace lambdamusim
} // namespace uiiit
