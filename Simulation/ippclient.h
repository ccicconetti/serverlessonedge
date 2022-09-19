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

#include <queue>

namespace uiiit {
namespace simulation {

/**
 * Client modeling an Interrupted Poisson Process: ON and OFF phases alternate,
 * each drawn from exponentially-distributed random variables; during the ON
 * phases lambda requests and generated with a random interval drawn from a
 * uniform r.v.; the size of the lambda requests is also extracted from an
 * independent uniform r.v.
 */
class IppClient final : public Client
{
  enum class State : int { On = 0, Off = 1 };

 public:
  /**
   * \throw std::runtime_error if aShortSleepMin >= aShortSleepMax or
   * aShortSleepMax == 0
   */
  explicit IppClient(const double                 aTimeOn,
                     const double                 aTimeOff,
                     const double                 aShortSleepMin,
                     const double                 aShortSleepMax,
                     const size_t                 aSizeMin,
                     const size_t                 aSizeMax,
                     const size_t                 aSeedUser,
                     const size_t                 aSeedInc,
                     const size_t                 aNumRequests,
                     const std::set<std::string>& aServers,
                     const bool                   aSecure,
                     const support::Conf&         aClientConf,
                     const std::string&           aLambda,
                     const support::Saver&        aSaver,
                     const bool                   aDry);

  /**
   * \throw std::runtime_error if aShortSleepMin >= aShortSleepMax or
   * aShortSleepMax == 0
   */
  explicit IppClient(const double                 aTimeOn,
                     const double                 aTimeOff,
                     const double                 aShortSleepMin,
                     const double                 aShortSleepMax,
                     const std::vector<size_t>&   aSizes,
                     const size_t                 aSeedUser,
                     const size_t                 aSeedInc,
                     const size_t                 aNumRequests,
                     const std::set<std::string>& aServers,
                     const bool                   aSecure,
                     const support::Conf&         aClientConf,
                     const std::string&           aLambda,
                     const support::Saver&        aSaver,
                     const bool                   aDry);

  size_t simulate(const double aDuration) override;

 private:
  //! Initializes everything except the lambda request size generation.
  explicit IppClient(const double                 aTimeOn,
                     const double                 aTimeOff,
                     const double                 aShortSleepMin,
                     const double                 aShortSleepMax,
                     const size_t                 aSeedUser,
                     const size_t                 aSeedInc,
                     const size_t                 aNumRequests,
                     const std::set<std::string>& aServers,
                     const bool                   aSecure,
                     const support::Conf&         aClientConf,
                     const std::string&           aLambda,
                     const support::Saver&        aSaver,
                     const bool                   aDry);

  size_t loop() override;

 private:
  const double           theTimeOn;
  const double           theTimeOff;
  const double           theShortSleepMin;
  const double           theShortSleepMax;
  support::ExponentialRv theOnDist;
  support::ExponentialRv theOffDist;
  support::UniformRv     theSleepDist;
  std::queue<double>     theBatch;
  State                  theState;
  double                 theNextStateChange;
  support::Chrono        theOnChrono;
};

} // namespace simulation
} // namespace uiiit
