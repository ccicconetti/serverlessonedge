/*
 ___ ___ __     __ ____________
|   |   |  |   |__|__|__   ___/   Ubiquitout Internet @ IIT-CNR
|   |   |  |  /__/  /  /  /    C++ edge computing libraries and tools
|   |   |  |/__/  /   /  /  https://bitbucket.org/ccicconetti/edge_computing/
|_______|__|__/__/   /__/

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2018 Claudio Cicconetti <https://about.me/ccicconetti>

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

#include "edgecontroller.h"
#include "edgecontrollerinstaller.h"

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace uiiit {
namespace edge {

class Topology;

/**
 * Edge controller announcing computers to routers in a hierarchical manner,
 * depending on the network topology provided as input.
 *
 * Behavior is as follows:
 *
 * - When a new computer is announced, its home router is identified from
 *   the topology. Then, the computer is announced as final destination to
 *   the home router only, while the latter is announced to all the other
 *   routers as an intermediate step to reach the computer.
 *
 * - When a computer is removed, it is removed from the forwarding table
 *   of its home router, while on all the other routers the entry announced
 *   for the home router is removed only if there are no other lambdas
 *   served by the home router.
 *
 * - When a router is added or removed (the latter as a result of a
 *   failed communication when adding/removing a forwarding table entry),
 *   then all the routers' forwarding tables are flushed and then
 *   re-created from scratch with a brand new configuration obtained
 *   by adding one a time all the lambdas offered from all the computers.
 *
 * The actual announce/removal of routes is left to further derived classes.
 */
class EdgeControllerHier : public EdgeController,
                           virtual public EdgeControllerInstaller
{
 public:
  enum class Objective {
    MinMax = 0,
    MinAvg = 1,
  };

  /**
   * Build an uninitialized controller.
   */
  explicit EdgeControllerHier();

  virtual ~EdgeControllerHier() override;

  /**
   * Set a given objective function. Cannot be called twice.
   * Must be called before the object can be considered fully initialized.
   *
   * \param aObjective The objective function to determine the home
   *        e-router. If it is MinMax, then we select the e-router
   *        that minimizes the maximum cost (from the topology)
   *        from any other e-router to the candidate home e-router
   *        to the target e-computer, breaking ties by looking to
   *        the average of such values. If it is MinAvg, then the
   *        policy is the opposite: minimize average first, break
   *        ties with maximum cost.
   */
  void objective(const Objective aObjective);

  /**
   * Set a given topology. Cannot be called twice.
   * Must be called before the object can be considered fully initialized.
   */
  void loadTopology(std::unique_ptr<Topology>&& aTopology);

 private:
  void addComputer(const std::string&            aEndpoint,
                   const std::list<std::string>& aLambdas) override {
  }

  void delComputer(const std::string&            aEndpoint,
                   const std::list<std::string>& aLambdas) override {
  }

  void addLambda(const std::string& aLambda) override {
  }

  void delLambda(const std::string& aLambda) override {
  }

  void privateAnnounceComputer(const std::string&   aEdgeServerEndpoint,
                               const ContainerList& aContainers) override;

  void privateAnnounceRouter(const std::string& aEdgeServerEndpoint,
                             const std::string& aEdgeRouterEndpoint) override;

  void privateRemoveComputer(const std::string&            aEdgeServerEndpoint,
                             const std::list<std::string>& aLambdas) override;
  /**
   * Remove the router with the given end-points.
   *
   * Called because the router did not respond on the forwarding table
   * interface during a table update.
   * This function calls the reset() method.
   */
  void privateRemoveRouter(const RouterEndpoints& aRouterEndpoints) override;

  /**
   * We use the following formula to rank the routers' addresses.
   *
   * <pre>
   *      /                                             \
   *     |        /               \                      |
   * min | Omega | d_ic + max d_ji | + N d_ic + sum d_ji |
   *  i  |        \        j      /                      |
   *      \                                             /
   * </pre>
   *
   * where:
   * - R: set of routers' addresses
   * - N: number of nodes in the topology
   * - d_ij: distance between node i (src) and node j (dst)
   * - Omega > 2* |N|^2
   *
   * \return the end-point of the edge router that is closest to the given
   *         computer, or an empty string if there is no edge router.
   *
   * We use a lazy initialization pattern: if the computer address is not
   * found in theClosest data structure then we compute it from the topology and
   * it, otherwise it is returned immediately.
   *
   * \throw std::runtime_error if the topology has not been loaded.
   */
  std::string findClosest(const std::string& aComputerAddress);

  /**
   * \return the end-points of the router located at the given address.
   *         If there are multiple edge routers co-located at the same
   *         address, then a random one is returned.
   *
   * \pre aRouterAddress is known as a router.
   */
  RouterEndpoints routerEndpoints(const std::string& aRouterAddress) const;

  /**
   * Reset all the forwarding tables of all the known routers.
   * Invoked as the set of routers changes.
   */
  void reset();

  /**
   * \return the address part of the endpoint address:port.
   *
   * \throw std::runtime_error if the schema is invalid.
   */
  static std::string address(const std::string& aEndpoint);

 private:
  std::unique_ptr<const Objective>           theObjective;
  std::unique_ptr<Topology> theTopology;

  // key:   edge router address
  // value: vector of edge router end-points at that address
  std::map<std::string, std::vector<RouterEndpoints>> theRouterAddresses;

  // key:   computer address
  // value: router address
  std::map<std::string, std::string> theClosest;

  // key:   edge server router end-point
  // value: map of
  //        key:   lambda name
  //        value: set of computers offering this lambda and announced
  //               via the current router in the key
  std::map<std::string, std::map<std::string, std::set<std::string>>>
      theAnnouncedLambdas;

  // key:   end-point of the lambda processing server
  // value: end-point of the forwarding table configuration server
  std::map<std::string, std::string> theForwardingTableEndpoints;
};

EdgeControllerHier::Objective objectiveFromString(const std::string& aValue);

std::string toString(const EdgeControllerHier::Objective aObjective);

} // namespace edge
} // namespace uiiit
