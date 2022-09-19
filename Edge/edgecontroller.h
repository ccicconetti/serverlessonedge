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

#include "Support/macros.h"
#include "edgecontrollermessages.h"

#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <tuple>

namespace uiiit {
namespace edge {

/**
 * Thread-safe controller keeping a list of the edge routers and computers.
 *
 * The actual logic is to be implemented by derived classes reacting to
 * the private virtual functions defined here.
 */
class EdgeController
{
 protected:
  struct RouterEndpoints {
    /**
     * \param aEdgeServer the end-point that serves lambda functions.
     * \param aForwardingTableServer the end-point that allows the router's
     *        forwarding table to be configured.
     */
    explicit RouterEndpoints(const std::string& aEdgeServer,
                             const std::string& aForwardingTableServer)
        : theEdgeServer(aEdgeServer)
        , theForwardingTableServer(aForwardingTableServer) {
    }

    std::string theEdgeServer;
    std::string theForwardingTableServer;

    bool operator==(const RouterEndpoints& aOther) const noexcept {
      return theEdgeServer == aOther.theEdgeServer and
             theForwardingTableServer == aOther.theForwardingTableServer;
    }

    bool operator<(const RouterEndpoints& aOther) const noexcept {
      return theEdgeServer < aOther.theEdgeServer or
             (theEdgeServer == aOther.theEdgeServer and
              theForwardingTableServer < aOther.theForwardingTableServer);
    }
  };

  class Computers
  {
   public:
    enum class AddStatus {
      AlreadyPresent    = 0,
      NotPresent        = 1,
      ContainersChanged = 2,
    };

    /**
     * Add a new computer, with associate a set of containers.
     *
     * \return AlreadyPresent if the computer is already known, with exactly the
     *         same set of containers; NotPresent if the computer is not known;
     *         ContainersChanged means that there is already a computer with the
     *         same end-point but a different set of containers, in which case
     *         the data structure is not updated.
     */
    AddStatus add(const std::string&   aEdgeServerEndpoint,
                  const ContainerList& aContainers);

    /**
     * Remove a computer.
     *
     * \return the list of lambdas offered by the computer, if any.
     */
    std::list<std::string> remove(const std::string& aEdgeServerEndpoint);

    //! \return a list of all the <lambda, destination> pairs.
    std::list<std::pair<std::string, std::string>> lambdas() const;

    //! \return all computers.
    const std::map<std::string, ContainerList>& computers() const;

    //! Print a ASCII representation of the computers.
    void print(std::ostream& aStream) const;

   private:
    std::map<std::string, ContainerList> theComputers;
  };

  class Routers
  {
   public:
    /**
     * Add a new router, identified by its two end-points, i.e. for the lambda
     * processor and forwarding table servers, respectively.
     *
     * \param aEdgeServerEndpoint The lambda processor server end-point.
     * \param aEdgeRouterEndpoint The forwarding table server end-point.
     */
    void add(const std::string& aEdgeServerEndpoint,
             const std::string& aEdgeRouterEndpoint);

    /**
     * Remove a router identified by its lambda processor end-point.
     *
     * \return true if actually removed.
     */
    bool remove(const std::string& aEdgeServerEndpoint);

    //! \return all router end-points.
    const std::map<std::string, std::string>& routers() const noexcept;

    //! \return the forwarding server end-point or an empty string if unlisted.
    std::string
    forwardingServerEndpoint(const std::string& aEdgeServerEndpoint) const;

    //! Print a ASCII representation of the routers..
    void print(std::ostream& aStream) const;

   private:
    // key: edge server end-point, value: forwarding server end-point
    std::map<std::string, std::string> theRouters;
  };

  // #0: lambda name
  // #1: lambda processor end-point (computer or intermediate router)
  // #2: weight
  // #3: final flag
  using Entries = std::list<std::tuple<std::string, std::string, float, bool>>;

 public:
  NONCOPYABLE_NONMOVABLE(EdgeController);

  //! Create a controller with no entries.
  explicit EdgeController();

  virtual ~EdgeController() {
  }

  /**
   * Announce a new computer. Notify all routers.
   *
   * If another computer with the same end-point exists it is updated.
   */
  void announceComputer(const std::string&   aEdgeServerEndpoint,
                        const ContainerList& aContainers);

  /**
   * Announce a new router. Notify all computers to it.
   *
   * If another router with the same end-point exists it is updated.
   */
  void announceRouter(const std::string& aEdgeServerEndpoint,
                      const std::string& aEdgeRouterEndpoint);

  /**
   * Remove this computer from the known set.
   */
  void removeComputer(const std::string& aEdgeServerEndpoint);

  void print(std::ostream& aStream) const;

 protected:
  /**
   * Remove the router with the given end-points.
   *
   * Called because the router did not respond on the forwarding table
   * interface during a table update.
   */
  void removeRouter(const std::string& aRouterEndpoint);

  //! Invoked as a new computer is added.
  virtual void addComputer(const std::string&            aEndpoint,
                           const std::list<std::string>& aLambdas) = 0;

  //! Invoked as a computer is terminated.
  virtual void delComputer(const std::string&            aEndpoint,
                           const std::list<std::string>& aLambdas) = 0;

  //! Invoked as a new lambda becomes known to the controller.
  virtual void addLambda(const std::string& aLambda) = 0;

  //! Invoked as the last computer serving a lambda disappears.
  virtual void delLambda(const std::string& aLambda) = 0;

  //! Implemented by derived classes to implement logic to announce a computer.
  virtual void privateAnnounceComputer(const std::string&   aEdgeServerEndpoint,
                                       const ContainerList& aContainers) = 0;

 private:
  //! Called by removeComputer() and announceComputer().
  void privateRemoveComputer(const std::string& aEdgeServerEndpoint);

  //! Implemented by derived classes to implement logic to announce a router.
  virtual void
  privateAnnounceRouter(const std::string& aEdgeServerEndpoint,
                        const std::string& aEdgeRouterEndpoint) = 0;

  //! Implemented by derived classes to implement logic to remove a computer.
  virtual void
  privateRemoveComputer(const std::string&            aEdgeServerEndpoint,
                        const std::list<std::string>& aLambdas) = 0;

  //! Implemented by derived classes to implement logic to remove a router.
  virtual void privateRemoveRouter(const std::string& aRouterEndpoint) = 0;

 protected:
  using LambdaMap = std::map<std::string, std::set<std::string>>;

  mutable std::mutex theMutex;
  Computers          theComputers;
  Routers            theRouters;
  LambdaMap          theLambdas;
};

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                      aStream,
                         const uiiit::edge::EdgeController& aController);
