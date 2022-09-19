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

#include "Edge/edgecontroller.h"
#include "EtsiMec/appinfo.h"
#include "EtsiMec/grpcueapplcmproxyclient.h"
#include "Support/macros.h"
#include "Support/periodictask.h"

#include <list>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <tuple>

namespace uiiit {
namespace edge {

/**
 * Connects to a StaticUeAppLcmProxy via its gRPC interface to notify:
 * - lambda functions, i.e. UE applications in ETSI terminology;
 * - availability of edge computers;
 * - associations for default and individual routes for clients and lambdas.
 *
 * Whenever a new edge computer registers to the controller, its lambdas are
 * noted. If there are lambdas for which there is currently no edge computer,
 * then a new default route is notified to the LCM Proxy for those lambdas only.
 *
 * If an edge computer deregisters itself from the controller, then we check for
 * each of its previously served lambdas if that is the default router for the
 * lambda: if this is the case, a new edge computer is picked as the default
 * router, otherwise nothing is done about it. Furthermore, all individual
 * routers having the deregistered edge computer are removed, as well (the LCM
 * proxy will have to fall-back to the default edge router for them).
 *
 * Periodically individual routes are optimized so as to distribute as evenly as
 * possible the clients with an active context among the available edge
 * computers, for each lambda. New edge clients creating a context in between
 * two consecutive optimizations will always use the default edge router.
 *
 * This class assumes to be the only element controlling the LCM proxy; if this
 * is not the case, then invariants will not hold and the behavior is
 * unpredictable, possibly also leading to the LCM proxy having an inconsistent
 * state.
 *
 * This data structure is non-copyable and non-movable.
 * Access to the public methods is not thread-safe.
 */
class EdgeControllerEtsiBalanced final : public EdgeController
{
  NONCOPYABLE_NONMOVABLE(EdgeControllerEtsiBalanced);
  FRIEND_TEST(TestEdgeControllerEtsiBalanced, test_table);

  struct Desc {
    //! Initialize with the given edge computer as the default and only one.
    explicit Desc(const std::string& aComputer)
        : theDefault(aComputer)
        , theComputers({aComputer}) {
    }

    std::string           theDefault;
    std::set<std::string> theComputers;
  };
  // key:   lambda name
  // value: descriptor containig
  //        - the current default edge router's end-point
  //        - the set of possible end-points of edge routers serving the lambda
  using DescMap = std::map<std::string, Desc>;

  // list of elements:
  // 0: edge client address
  // 1: lambda function name
  // 2: edge computer end-point
  // all three strings are always non-empty
  using Table = std::list<std::tuple<std::string, std::string, std::string>>;

 public:
  /**
   * \param aEndpoint The gRPC end-point of the UE application LCM proxy.
   * \param aPeriod The optimization period, in fractional seconds.
   */
  explicit EdgeControllerEtsiBalanced(const std::string& aEndpoint,
                                      const double       aOptimizationPeriod);

 private:
  void addComputer(const std::string&            aEndpoint,
                   const std::list<std::string>& aLambdas) override;

  void delComputer(const std::string&            aEndpoint,
                   const std::list<std::string>& aLambdas) override;

  void addLambda(const std::string& aLambda) override;

  void delLambda(const std::string& aLambda) override;

  void privateAnnounceComputer(const std::string&,
                               const ContainerList&) override {
  }

  void privateAnnounceRouter(const std::string&, const std::string&) override {
  }

  void privateRemoveComputer(const std::string&,
                             const std::list<std::string>&) override {
  }
  void privateRemoveRouter(const std::string&) override {
  }

  //! Run periodically to optimize individual edge client computer associations.
  void optimize();

  //! Used within optimize.
  static Table table(const Table& aTable, const DescMap& aDesc);

 private:
  mutable std::mutex               thePeriodicMutex;
  etsimec::GrpcUeAppLcmProxyClient theClient;
  DescMap                          theDesc;
  std::set<std::string>            theComputerEndpoints;
  Table                            theLastTableReceived;
  support::PeriodicTask            thePeriodicTask;
};

} // namespace edge
} // namespace uiiit
