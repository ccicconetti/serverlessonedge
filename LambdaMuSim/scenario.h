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

#include "LambdaMuSim/appmodel.h"
#include "LambdaMuSim/appperiods.h"
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
  std::size_t theNumContainers    = 0;
  std::size_t theTotCapacity      = 0;
  double      theNumLambda        = 0; //!< int with S, real with D
  double      theNumMu            = 0; //!< int with S, real with D
  double      theLambdaCost       = 0; //!< average with D
  double      theMuCost           = 0; //!< average with D
  double      theMuCloud          = 0; //!< int with S, real with D
  std::size_t theMuMigrations     = 0; //!< D only
  std::size_t theNumOptimizations = 0; //!< D only

  bool operator==(const PerformanceData& aOther) const noexcept;

  std::vector<std::string>               toStrings() const;
  static const std::vector<std::string>& toColumns();
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
    App(const ID   aBroker,
        const Type aType,
        const long aServiceRate,
        const long aExchangeSize,
        const long aStorageSize)
        : theBroker(aBroker)
        , theType(aType)
        , theServiceRate(aServiceRate)
        , theExchangeSize(aExchangeSize)
        , theStorageSize(aStorageSize) {
      // noop
    }

    ID   theBroker       = 0; //!< the broker to which this app connects
    Type theType         = Type::Lambda; //!< the current app type
    long theServiceRate  = 1;            //!< the rate of function invocations
    long theExchangeSize = 1; //!< amount of data units per invocation
    long theStorageSize  = 1; //!< state size in the remote storage
    ID   theEdge         = 0; //!< the edge to which is assigned (mu-only)
    std::vector<std::pair<ID, double>> theWeights; //!< weights (lambda-only)

    std::string toString() const;
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
   * @param aAppModel The model of the applications' parameters.
   *
   * @throw std::runtime_error if the input args are inconsistent.
   */
  explicit Scenario(
      statesim::Network& aNetwork,
      const double       aCloudDistanceFactor,
      const std::function<std::size_t(const statesim::Node&)>& aNumContainers,
      const std::function<long(const statesim::Node&)>& aContainerCapacity,
      AppModel&                                         aAppModel);

  /**
   * @brief Change the model of the applications' parameters.
   *
   * @param aAppModel The new model.
   */
  void appModel(AppModel& aAppModel);

  /**
   * @brief Run a single snapshot, overwriting previous apps/allocations.
   *
   * @param aAvgLambda The average number of lambda apps.
   * @param aAvgMu The average number of mu apps.
   * @param aAlpha The lambda reservation factor.
   * @param aBeta  The lambda overprovisioning factor.
   * @param aSeed The RNG seed.
   * @return PerformanceData
   *
   * @throw std::runtime_error if the arguments are invalid.
   */
  PerformanceData snapshot(const std::size_t aAvgLambda,
                           const std::size_t aAvgMu,
                           const double      aAlpha,
                           const double      aBeta,
                           const std::size_t aSeed);

  /**
   * @brief Run a dynamic simulation.
   *
   * @param aDuration The simulation duration.
   * @param aWarmUp The warm-up duration.
   * @param aEpoch The duration of an epoch.
   * @param aPeriods The application periods.
   * @param aAvgApps The average number of apps.
   * @param aAlpha The lambda reservation factor.
   * @param aBeta  The lambda overprovisioning factor.
   * @param aSeed The RNG seed.
   * @return PerformanceData
   *
   * @throw std::runtime_error if the arguments are invalid.
   */
  PerformanceData dynamic(const double                           aDuration,
                          const double                           aWarmUp,
                          const double                           aEpoch,
                          const std::vector<std::deque<double>>& aPeriods,
                          const std::size_t                      aAvgApps,
                          const double                           aAlpha,
                          const double                           aBeta,
                          const std::size_t                      aSeed);

 private:
  double&       networkCost(const ID aBroker, const ID aEdge) noexcept;
  const double& networkCost(const ID aBroker, const ID aEdge) const noexcept;
  std::string   networkCostToString() const;
  std::string   appsToString() const;
  static std::string toString(const Type aType);
  void               assignMuApps(const double aAlpha, PerformanceData& aData);
  void assignLambdaApps(const double aBeta, PerformanceData& aData);
  std::pair<double, double> migrateLambdaToMu(const ID     aApp,
                                              const double aAlpha);
  std::pair<double, double> migrateMuToLambda(const ID aApp);
  double                    lambdaCost(const ID aApp) const;
  double                    muCost(const ID aApp) const;
  static void               checkArgs(const double aAlpha, const double aBeta);
  static Type               flip(const Type aType) noexcept;
  void                      clearPreviousAssignments(PerformanceData& aData);

 private:
  AppModel* theAppModel; // initialized in the ctor, can be changed at run-time

  std::vector<App>    theApps;        // size = A
  std::vector<Broker> theBrokers;     // size = B
  std::vector<Edge>   theEdges;       // size = E
  std::vector<double> theNetworkCost; // matrix BxE
                                      // b1e1 ... b1eE b2e1 ... b2eN etc.
};

} // namespace lambdamusim
} // namespace uiiit
