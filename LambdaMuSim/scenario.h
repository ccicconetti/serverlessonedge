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

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace uiiit {

namespace statesim {
class Network;
class Node;
} // namespace statesim

namespace lambdamusim {

struct PerformanceData {
  double      theLambdaCost = 0;
  double      theMuCost     = 0;
  std::size_t theMuCloud    = 0;

  std::vector<std::string> toStrings() const;
};

class Scenario
{
  NONCOPYABLE_NONMOVABLE(Scenario);

  enum {
    CLOUD = 0,
  };

  using ID = unsigned long;
  enum class Type : uint16_t {
    Lambda = 0,
    Mu     = 1,
  };

  struct App {
    App(const ID aBroker, const Type aType)
        : theBroker(aBroker)
        , theType(aType) {
      // noop
    }

    ID   theBroker = 0;            //!< the broker to which this app connects
    Type theType   = Type::Lambda; //!< the current app type
    ID   theEdge   = 0;            //!< the edge to which is assigned (mu-only)
  };

  struct Broker {
    Broker()
        : theApps() {
      // noop
    }
    std::vector<ID> theApps;
  };

  struct Edge {
    std::size_t     theNumContainers     = 0;
    long            theContainerCapacity = 0;
    std::vector<ID> theLambdaApps;
    std::vector<ID> theMuApps;
  };

 public:
  /**
   * @brief Construct a new scenario.
   *
   * @param aNetwork The network to use to determine the edge costs.
   * @param aCloudDistanceFactor Factor to scale the maximum distance in
   * aNetwork to obtain the cost to reach the cloud
   * @param aNumContainers Function to determine the number of containers based
   * on the node characteristics.
   * @param aContainerCapacity Function to determine the capacity of
   * lambda-containers based on the node characteristics.
   *
   * @throw std::runtime_error if the input args are inconsistent.
   */
  explicit Scenario(
      statesim::Network& aNetwork,
      const double       aCloudDistanceFactor,
      const std::function<std::size_t(const statesim::Node&)>& aNumContainers,
      const std::function<long(const statesim::Node&)>& aContainerCapacity);

  /**
   * @brief Run a single snapshot, overwriting previous apps/allocations.
   *
   * @param aAvgLambda The average number of lambda apps.
   * @param aAvgMu The average number of mu apps.
   * @param aAlpha The lambda reservation factor.
   * @param aBeta  The lambda overprovisioning factor.
   * @param aLambdaRequest The capacity requested by each lambda app.
   * @param aSeed The RNG seed.
   * @return PerformanceData
   *
   * @throw std::runtime_error if alpha,beta not in [0,1]
   */
  PerformanceData snapshot(const std::size_t aAvgLambda,
                           const std::size_t aAvgMu,
                           const double      aAlpha,
                           const double      aBeta,
                           const long        aLambdaRequest,
                           const std::size_t aSeed);

 private:
  double&       networkCost(const ID aBroker, const ID aEdge) noexcept;
  const double& networkCost(const ID aBroker, const ID aEdge) const noexcept;
  std::string   networkCostToString() const;
  std::string   appsToString() const;
  static std::string toString(const Type aType);

 private:
  std::vector<App>    theApps;        // size = A
  std::vector<Broker> theBrokers;     // size = B
  std::vector<Edge>   theEdges;       // size = E
  std::vector<double> theNetworkCost; // matrix BxE
                                      // b1e1 ... b1eE b2e1 ... b2eN etc.
};

} // namespace lambdamusim
} // namespace uiiit
