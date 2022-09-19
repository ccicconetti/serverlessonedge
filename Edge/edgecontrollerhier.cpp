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

#include "edgecontrollerhier.h"

#include "Edge/topology.h"
#include "Support/random.h"
#include "Support/tostring.h"

#include <glog/logging.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <utility>

namespace uiiit {
namespace edge {

EdgeControllerHier::EdgeControllerHier()
    : EdgeController()
    , theObjective(nullptr)
    , theTopology(nullptr)
    , theRouterAddresses()
    , theClosest()
    , theAnnouncedLambdas()
    , theForwardingTableEndpoints() {
  LOG(INFO) << "Created a controller with hierarchical routing";
}

EdgeControllerHier::~EdgeControllerHier() {
  LOG_IF(WARNING, theObjective == nullptr or theTopology == nullptr)
      << "Suspiciously destroying an uninitialized hierarchical edge "
         "controller";
}

void EdgeControllerHier::objective(const Objective aObjective) {
  if (theObjective) {
    throw std::runtime_error(
        "Objective already set, cannot be changed afterwards");
  }
  theObjective = std::make_unique<const Objective>(aObjective);
  LOG(INFO) << "Set the following objective in the controller: "
            << toString(aObjective);
}

void EdgeControllerHier::loadTopology(std::unique_ptr<Topology>&& aTopology) {
  if (theTopology) {
    throw std::runtime_error(
        "Topology already loaded, cannot be changed afterwards");
  }
  theTopology = std::forward<std::unique_ptr<Topology>>(aTopology);
  LOG(INFO) << "Loaded the following topology in the controller:\n"
            << *theTopology;
}

void EdgeControllerHier::privateAnnounceComputer(
    const std::string& aEdgeServerEndpoint, const ContainerList& aContainers) {
  ASSERT_IS_LOCKED(theMutex);

  // nothing to be done if there are no edge routers
  if (theRouterAddresses.empty()) {
    return;
  }

  const auto myHomeRouterAddress = findClosest(address(aEdgeServerEndpoint));
  assert(not myHomeRouterAddress.empty());

  const auto myRouterEndpoint = routerEndpoint(myHomeRouterAddress);
  assert(not myRouterEndpoint.empty());

  //
  // announce the new computer as the final destination on the home router
  //
  Entries myEntries;
  for (const auto& myContainer : aContainers.theContainers) {
    myEntries.push_back(std::make_tuple(
        myContainer.theLambda, aEdgeServerEndpoint, 1.0f, true));
  }

  const auto myForwardingServerEndpoint =
      theRouters.forwardingServerEndpoint(myRouterEndpoint);
  if (not myForwardingServerEndpoint.empty()) {
    if (not changeRoutes(myForwardingServerEndpoint, myEntries)) {
      removeRouter(myRouterEndpoint);
      // no need to continue the body of the function since the call
      // to removeRouter() will also cause all the forwarding tables
      // to be reset
      return;
    }
  }

  //
  // announce the home router as the intermediate destination to
  // reach the new computer, unless already announced
  //
  auto& myLambdas = theAnnouncedLambdas[myRouterEndpoint];
  for (auto it = myEntries.begin(); it != myEntries.end(); /* incr in loop */) {
    const auto& myLambda    = std::get<0>(*it);
    auto&       myComputers = myLambdas[myLambda];

    const auto myInserted = myComputers.insert(aEdgeServerEndpoint).second;
    LOG_IF(WARNING, not myInserted)
        << "Announcing again the same computer at " << aEdgeServerEndpoint
        << " for lambda " << myLambda;

    if (not myInserted or myComputers.size() > 1) {
      // we remove this entry from the list of announcements to make
      // to the routers (except home) if one of the two is true:
      // 1. this computer was already present in the set of computers
      //    offering this lambda and having the same home router
      // 2. if there are at least two computers announced for this lambda
      //    already, which means that this router has been already announced
      //    for this lambda
      it = myEntries.erase(it);

    } else {
      assert(myComputers.size() == 1);
      // otherwise, if this lambda has never been announced by the current
      // router, then we change the destination from the computer to the
      // edge server end-point of the router, clear the final flag and
      // announce it to all the other routers
      std::get<1>(*it) = myRouterEndpoint;
      // std::get<2> is the weight: it remains set to the default value 1.0
      std::get<3>(*it) = false;
      ++it;
    }
  }

  // notify the lambdas to all the routers, except the home router
  for (const auto& myRouter : theRouters.routers()) {
    // skip home router and do not announce an empty list of routes
    if (myRouter.first == myRouterEndpoint or myEntries.empty()) {
      continue;
    }
    if (not changeRoutes(myRouter.second, myEntries)) {
      removeRouter(myRouter.first);
      // no need to continue the loop: the call to removeRouter() will also
      // cause all the forwarding tables to be reset
      return;
    }
  }
}

void EdgeControllerHier::privateAnnounceRouter(
    const std::string& aEdgeServerEndpoint,
    const std::string& aEdgeRouterEndpoint) {
  ASSERT_IS_LOCKED(theMutex);

  // add the router to the map of address -> endpoints
  auto it = theRouterAddresses.emplace(address(aEdgeServerEndpoint),
                                       std::vector<std::string>());

  // invalidate theClosest data structure if a new address has been added
  if (it.second) {
    theClosest.clear();
  }

  it.first->second.emplace_back(aEdgeServerEndpoint);

  // add the router to the map of edge server -> forwarding table end-points
  [[maybe_unused]] const auto myInserted =
      theForwardingTableEndpoints
          .emplace(aEdgeServerEndpoint, aEdgeRouterEndpoint)
          .second;
  assert(myInserted);

  // reset the forwarding tables of all the routers
  reset();
}

void EdgeControllerHier::privateRemoveComputer(
    const std::string&            aEdgeServerEndpoint,
    const std::list<std::string>& aLambdas) {
  ASSERT_IS_LOCKED(theMutex);

  struct RemoveElem {
    explicit RemoveElem(const std::string& aLambdaProcessorServer,
                        const std::string& aForwardingTableServer,
                        const std::string& aEdgeServer,
                        const std::string& aLambda)
        : theLambdaProcessorServer(aLambdaProcessorServer)
        , theForwardingTableServer(aForwardingTableServer)
        , theEdgeServer(aEdgeServer)
        , theLambda(aLambda) {
    }

    std::string theLambdaProcessorServer;
    std::string theForwardingTableServer;
    std::string theEdgeServer;
    std::string theLambda;
  };

  if (VLOG_IS_ON(3)) {
    std::stringstream myStream;
    for (const auto& it : theAnnouncedLambdas) {
      for (const auto& jt : it.second) {
        myStream << "router " << it.first << ", lambda " << jt.first
                 << ", computers " << ::toString(jt.second, ", ") << '\n';
      }
    }
    LOG(INFO) << "### lambdas directly reached by routers\n" << myStream.str();
  }

  std::list<RemoveElem> myRemoveElems;
  for (const auto& myLambda : aLambdas) {
    // find the <lambda, computer> in the announced lambdas structure
    // the computer should appear exactly in one point
    for (auto it = theAnnouncedLambdas.begin(); it != theAnnouncedLambdas.end();
         ++it) {
      auto jt = it->second.find(myLambda);
      if (jt == it->second.end()) {
        continue;
      }
      assert(not jt->second.empty());
      const auto myErased = jt->second.erase(aEdgeServerEndpoint);
      if (myErased > 0) {
        assert(myErased == 1);

        // we must always remove the (final) entry from the home router
        const auto myHomeForwardingTableIt =
            theForwardingTableEndpoints.find(it->first);
        assert(myHomeForwardingTableIt != theForwardingTableEndpoints.end());
        myRemoveElems.emplace_back(it->first,
                                   myHomeForwardingTableIt->first,
                                   aEdgeServerEndpoint,
                                   myLambda);

        // then, if the set of computers also becomes empty, then we must
        // announce to all the (other) routers that this router does not serve
        // anymore the current lambda, and we also remove the element from
        // the map
        if (jt->second.empty()) {

          // add one entry to be removed per router (except the home router)
          for (const auto& myEndpoints : theForwardingTableEndpoints) {
            if (myEndpoints.first == it->first) {
              // already added to the remove list
              continue;
            }
            myRemoveElems.emplace_back(
                myEndpoints.first,  // lambda processor server end-point
                myEndpoints.second, // forwarding table server end-point
                it->first,          // intermediate router server end-point
                myLambda);
          }

          // myLambda cannot be reached anymore by this router
          it->second.erase(jt);

          // also remove the whole router if this lambda was the last one
          if (it->second.empty()) {
            theAnnouncedLambdas.erase(it);
            // no need to update it since we break in a moment
          }
        }

        // we have found the computer, we can skip navigating the map further
        break;
      }
    }
  }

  // send out all removal annoucements
  for (const auto& myRemoveElem : myRemoveElems) {
    if (not removeRoutes(myRemoveElem.theForwardingTableServer,
                         myRemoveElem.theEdgeServer,
                         {myRemoveElem.theLambda})) {
      removeRouter(myRemoveElem.theLambdaProcessorServer);
      // no need to continue the loop: the call to removeRouter() will also
      // cause all the forwarding tables to be reset
      return;
    }
  }
}

std::string
EdgeControllerHier::findClosest(const std::string& aComputerAddress) {
  ASSERT_IS_LOCKED(theMutex);

  if (not theTopology) {
    throw std::runtime_error("Topology has not been loaded");
  }
  if (not theObjective) {
    throw std::runtime_error("Objective not set");
  }

  const auto it = theClosest.find(aComputerAddress);

  // no edge routers, return immediately emptry string
  if (theRouterAddresses.empty()) {
    return std::string();
  }

  // found, can be immediately returned
  if (it != theClosest.end()) {
    return it->second;
  }

  // not found, must be computed from the topology
  struct RankingElem {
    explicit RankingElem(const std::string& aAddress,
                         const double       aScore) noexcept
        : theAddress(aAddress)
        , theScore(aScore) {
    }
    bool operator<(const RankingElem& aOther) const noexcept {
      return theScore < aOther.theScore;
    }
    const std::string theAddress;
    const double      theScore;
  };
  const double myOmega =
      1.0 + 2.0 * theTopology->numNodes() * theTopology->numNodes();
  std::vector<RankingElem> myRanking;
  myRanking.reserve(theRouterAddresses.size());
  size_t myRouterCounter = 0;
  for (const auto& myRouterAddress : theRouterAddresses) {
    assert(not myRouterAddress.first.empty());

    double myMax = std::numeric_limits<double>::lowest();
    double mySum = 0;
    for (const auto& myOtherRouterAddress : theRouterAddresses) {
      const auto myOtherDistance = theTopology->distance(
          myRouterAddress.first, myOtherRouterAddress.first);
      myMax = std::max(myMax, myOtherDistance);
      mySum += myOtherDistance;
    }

    const auto myDistHomeToComp =
        theTopology->distance(myRouterAddress.first, aComputerAddress);

    const auto myMaxCost = myDistHomeToComp + myMax;
    const auto myAvgCost = theTopology->numNodes() * myDistHomeToComp + mySum;

    // the final score for ranking depends on the policy
    assert(theObjective);
    assert(*theObjective == Objective::MinMax or
           *theObjective == Objective::MinAvg);
    const auto myScore = *theObjective == Objective::MinMax ?
                             (myOmega * myMaxCost + myAvgCost) :
                             (myMaxCost + myOmega * myAvgCost);
    VLOG(2) << "scoring " << aComputerAddress << " #" << myRouterCounter << ' '
            << myRouterAddress.first << ' ' << myScore << " (= omega "
            << myOmega << ", min-max " << myMaxCost << ", min-avg " << myAvgCost
            << ")";
    myRanking.emplace_back(myRouterAddress.first, myScore);

    myRouterCounter++;
  }
  assert(not myRanking.empty());

  // find the router with minimum ranking
  const auto myMinElem = std::min_element(myRanking.begin(), myRanking.end());
  assert(myMinElem != myRanking.end());

  // update theClosest so that we don't have to re-do computation again
  // unless the set of router addresses (note: the topology _cannot_ change
  // by design, at the moment)
  theClosest.emplace(aComputerAddress, myMinElem->theAddress);

  return myMinElem->theAddress;
}

std::string
EdgeControllerHier::routerEndpoint(const std::string& aRouterAddress) const {
  ASSERT_IS_LOCKED(theMutex);

  const auto it = theRouterAddresses.find(aRouterAddress);
  assert(it != theRouterAddresses.end());

  if (it->second.size() == 1) {
    return it->second.front();
  }

  // return a random entry if there are multiple ones
  auto myRnd = it->second.size();
  while (myRnd == it->second.size()) {
    myRnd = static_cast<size_t>(support::random() * it->second.size());
    assert(myRnd <= it->second.size());
  }
  assert(myRnd < it->second.size());
  return it->second[myRnd];
}

std::string EdgeControllerHier::address(const std::string& aEndpoint) {
  const auto myPos = aEndpoint.find(":");
  if (myPos == std::string::npos or myPos == 0 or
      myPos == (aEndpoint.size() - 1)) {
    throw std::runtime_error("Invalid end-point: " + aEndpoint);
  }
  return aEndpoint.substr(0, myPos);
}

void EdgeControllerHier::privateRemoveRouter(
    const std::string& aRouterEndpoint) {
  ASSERT_IS_LOCKED(theMutex);

  //
  // update theRouterAddresses
  //
  const auto myAddress = address(aRouterEndpoint);
  auto       it        = theRouterAddresses.find(myAddress);
  assert(it != theRouterAddresses.end());
  for (auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
    if (*jt == aRouterEndpoint) {
      it->second.erase(jt);
      break;
    }
  }

  // if there are no more end-points associated to this address, remove it
  // altogether; in this case, we also invalidate theClosest data structure
  if (it->second.empty()) {
    theRouterAddresses.erase(it);
    theClosest.clear();
  }

  //
  // update theForwardingTableEndpoints
  //
  [[maybe_unused]] const auto myErased =
      theForwardingTableEndpoints.erase(aRouterEndpoint);
  assert(myErased == 1);

  //
  // reset the forwarding tables in all the routers
  //
  reset();
}

void EdgeControllerHier::reset() {
  ASSERT_IS_LOCKED(theMutex);

  // flush all entries of all routers
  for (const auto& myRouter : theForwardingTableEndpoints) {
    if (not flushRoutes(myRouter.second)) {
      removeRouter(myRouter.first);
      // no need to continue the loop: the call to removeRouter() will also
      // cause all the forwarding tables to be reset
      return;
    }
  }

  // clean up the map of lambdas announced
  theAnnouncedLambdas.clear();

  // add all lambdas one at a time
  for (const auto& myComputer : theComputers.computers()) {
    // the call to this function may cause the function reset() to be
    // called again, in case one router does not respond correctly when
    // contacted to update its forwarding table entries
    //
    // this process cannot recurse ad libitum because every time a
    // communication error with a router occurs the latter is removed,
    // hence eventually there will be no more routers and the
    // function will return immediately, thus terminating recursion
    privateAnnounceComputer(myComputer.first, myComputer.second);
  }
}

////////////////////////////////////////////////////////////////////////////////
// free functions

EdgeControllerHier::Objective objectiveFromString(const std::string& aValue) {
  if (aValue == "min-max") {
    return EdgeControllerHier::Objective::MinMax;
  } else if (aValue == "min-avg") {
    return EdgeControllerHier::Objective::MinAvg;
  }
  throw std::runtime_error("Invalid hierarchical controller objective: " +
                           aValue);
}

std::string toString(const EdgeControllerHier::Objective aObjective) {
  switch (aObjective) {
    case EdgeControllerHier::Objective::MinMax:
      return "min-max";
    case EdgeControllerHier::Objective::MinAvg:
      return "min-avg";
  }
  assert(false);
  return std::string();
}

} // namespace edge
} // namespace uiiit
