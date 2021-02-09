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

#include "client.h"

namespace uiiit {
namespace simulation {

/**
 * Client modeling the following process: the client periodically enters an
 * active phase in which it sends a number of lambda requests that is drawn from
 * a Poisson r.v.; the interval between consecutive lambda requests is drawn
 * from a uniform r.v.; the period duration is drawn from an exponentially
 * distributed r.v.; finally, the size of the lambda requests is also extracted
 * from an independent uniform r.v.
 */
class PoissonClient final : public Client
{
 public:
  explicit PoissonClient(const double                 aMeanPeriod,
                         const double                 aMeanBurstSize,
                         const double                 aShortSleepMin,
                         const double                 aShortSleepMax,
                         const size_t                 aSizeMin,
                         const size_t                 aSizeMax,
                         const size_t                 aSeedUser,
                         const size_t                 aSeedInc,
                         const size_t                 aNumRequests,
                         const std::set<std::string>& aServers,
                         const support::Conf&         aClientConf,
                         const std::string&           aLambda,
                         const support::Saver&        aSaver,
                         const bool                   aDry);

  explicit PoissonClient(const double                 aMeanPeriod,
                         const double                 aMeanBurstSize,
                         const double                 aShortSleepMin,
                         const double                 aShortSleepMax,
                         const std::vector<size_t>&   aSizes,
                         const size_t                 aSeedUser,
                         const size_t                 aSeedInc,
                         const size_t                 aNumRequests,
                         const std::set<std::string>& aServers,
                         const support::Conf&         aClientConf,
                         const std::string&           aLambda,
                         const support::Saver&        aSaver,
                         const bool                   aDry);

  size_t simulate(const double aDuration) override;

 private:
  //! Initializes everything except the lambda request size generation.
  explicit PoissonClient(const double                 aMeanPeriod,
                         const double                 aMeanBurstSize,
                         const double                 aShortSleepMin,
                         const double                 aShortSleepMax,
                         const size_t                 aSeedUser,
                         const size_t                 aSeedInc,
                         const size_t                 aNumRequests,
                         const std::set<std::string>& aServers,
                         const support::Conf&         aClientConf,
                         const std::string&           aLambda,
                         const support::Saver&        aSaver,
                         const bool                   aDry);

  size_t loop() override;

 private:
  support::ExponentialRv thePeriodDist;
  support::PoissonRv     theBurstSizeDist;
  support::UniformRv     theSleepDist;
  size_t                 theCurrentBurst;
  double                 theCurrentPeriod;
  support::Chrono        thePeriodChrono;
};

} // namespace simulation
} // namespace uiiit
