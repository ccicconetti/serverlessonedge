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

#include "Support/chrono.h"

#include <memory>
#include <vector>

namespace uiiit {

namespace support {
class RealRvInterface;
}

namespace simulation {

/**
 * A client issuing lambda requests at intervals drawn from a uniform,
 * constant, or exponential * distribution.
 */
class UnifClient final : public Client
{
  using RvPtr = std::unique_ptr<support::RealRvInterface>;

 public:
  enum class Distribution : int {
    CONSTANT    = 0,
    UNIFORM     = 1,
    EXPONENTIAL = 2,
  };

  explicit UnifClient(const std::vector<size_t>&   aSizes,
                      const double                 aInterRequestTime,
                      const Distribution           aDistribution,
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
  size_t loop() override;

 private:
  const RvPtr     theSleepDist;
  double          theLastTime;
  support::Chrono theChrono;
};

// free functions

UnifClient::Distribution distributionFromString(const std::string& aType);

} // namespace simulation
} // namespace uiiit
