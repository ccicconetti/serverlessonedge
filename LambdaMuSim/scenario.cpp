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

#include "LambdaMuSim/scenario.h"

#include "LambdaMuSim/mcfp.h"
#include "StateSim/network.h"
#include "StateSim/node.h"
#include "Support/random.h"
#include "hungarian-algorithm-cpp/Hungarian.h"

#include <glog/logging.h>

#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace uiiit {
namespace lambdamusim {

std::string toString(const hungarian::HungarianAlgorithm::DistMatrix& aMatrix) {
  std::stringstream ret;
  for (const auto& myColumns : aMatrix) {
    for (const auto& myValue : myColumns) {
      ret << '\t' << myValue;
    }
    ret << '\n';
  }
  return ret.str();
}

Scenario::Scenario(
    statesim::Network& aNetwork,
    const double       aCloudDistanceFactor,
    const std::function<std::size_t(const statesim::Node&)>& aNumContainers,
    const std::function<long(const statesim::Node&)>&        aContainerCapacity)
    : theApps()
    , theBrokers()
    , theEdges()
    , theNetworkCost() {

  if (aCloudDistanceFactor <= 0) {
    throw std::runtime_error("Invalid cloud distance factor, must be > 0: " +
                             std::to_string(aCloudDistanceFactor));
  }

  // load brokers: all the network clients
  std::vector<statesim::Node*> myBrokerPtrs;
  for (const auto& myClient : aNetwork.clients()) {
    myBrokerPtrs.emplace_back(myClient);
    theBrokers.emplace_back(Broker());
  }

  if (theBrokers.empty()) {
    throw std::runtime_error("Invalid network without brokers");
  }

  // add cloud node as theEdges[0], with values to be adjusted later:
  // - the number of container is the number of lambda+mu apps
  // - the capacity is the sum of requests of all lambda (depends on the apps)
  theEdges.emplace_back(Edge{0, 0});

  // load edge nodes: all the network processing nodes except clients
  std::vector<statesim::Node*> myEdgePtrs;
  myEdgePtrs.emplace_back(nullptr); // cloud node (not in Network)
  for (const auto& myProcessing : aNetwork.processing()) {
    if (std::find_if(myBrokerPtrs.begin(),
                     myBrokerPtrs.end(),
                     [myProcessing](const auto& aValue) {
                       return aValue == myProcessing;
                     }) == myBrokerPtrs.end()) {
      theEdges.emplace_back(Edge{aNumContainers(*myProcessing),
                                 aContainerCapacity(*myProcessing)});
      myEdgePtrs.emplace_back(myProcessing);
    }
  }

  if (theEdges.size() <= 1) {
    throw std::runtime_error("Invalid network without edge nodes");
  }

  // set the network cost
  theNetworkCost.resize(theBrokers.size() * theEdges.size());
  std::size_t myMaxDistance = 0;
  for (ID b = 0; b < theBrokers.size(); b++) {
    for (ID e = 1; e < theEdges.size(); e++) {
      assert(myBrokerPtrs[b] != nullptr);
      assert(myEdgePtrs[e] != nullptr);
      const auto myDistance = aNetwork.hops(*myBrokerPtrs[b], *myEdgePtrs[e]);
      myMaxDistance         = std::max(myMaxDistance, myDistance);
      networkCost(b, e)     = static_cast<double>(myDistance);
    }
  }
  const double myCloudDistance = aCloudDistanceFactor * myMaxDistance;
  for (ID b = 0; b < theBrokers.size(); b++) {
    networkCost(b, 0) = myCloudDistance;
  }
}

PerformanceData Scenario::snapshot(const std::size_t aAvgLambda,
                                   const std::size_t aAvgMu,
                                   const double      aAlpha,
                                   const double      aBeta,
                                   const long        aLambdaRequest,
                                   const std::size_t aSeed) {

  // consistency checks
  if (aAlpha < 0 or aAlpha > 1) {
    throw std::runtime_error("Invalid alpha, must be in [0,1]: " +
                             std::to_string(aAlpha));
  }
  if (aBeta < 0 or aBeta > 1) {
    throw std::runtime_error("Invalid beta, must be in [0,1]: " +
                             std::to_string(aBeta));
  }
  if (aLambdaRequest <= 0) {
    throw std::runtime_error("Invalid lambda request, must be > 0: " +
                             std::to_string(aLambdaRequest));
  }

  PerformanceData ret;

  support::UniformIntRv<ID> myBrokerRv(0, theBrokers.size() - 1, aSeed, 0, 0);
  const auto myNumLambda = support::PoissonRv(aAvgLambda, aSeed, 0, 0)();
  const auto myNumMu     = support::PoissonRv(aAvgMu, aSeed, 1, 0)();

  // clear apps and brokers and any previous assignment between edge nodes
  // and apps from previous calls
  theApps.clear();
  for (auto& myBroker : theBrokers) {
    myBroker.theApps.clear();
  }
  for (auto& myEdge : theEdges) {
    myEdge.theLambdaApps.clear();
    myEdge.theMuApps.clear();
  }

  // initialize apps and brokers
  for (std::size_t i = 0; i < myNumLambda; i++) {
    theApps.emplace_back(myBrokerRv(), Type::Lambda);
    theBrokers[theApps.back().theBroker].theApps.emplace_back(i);
  }

  for (std::size_t i = 0; i < myNumMu; i++) {
    theApps.emplace_back(myBrokerRv(), Type::Mu);
    theBrokers[theApps.back().theBroker].theApps.emplace_back(i);
  }

  // set the cloud num containers and capacity
  theEdges[CLOUD].theNumContainers     = 1 + myNumMu;
  theEdges[CLOUD].theContainerCapacity = myNumLambda * aLambdaRequest;

  VLOG(2) << "seed " << aSeed << ", " << myNumLambda << " lambda-apps, "
          << myNumMu << " mu-apps, network costs:\n"
          << networkCostToString();

  //
  // assign mu-apps to containers, taking into account the alpha factor
  // the capacities (and beta factor) are ignored
  //

  // prepare the containers
  std::vector<ID> myContainerToNodes;
  for (ID e = 0; e < theEdges.size(); e++) {
    // reduce the number of containers available for mu-apps, only for
    // edge-nodes (note this can go to zero)
    const auto myMuContainersAvailable =
        e == CLOUD ?
            theEdges[e].theNumContainers :
            static_cast<std::size_t>(theEdges[e].theNumContainers * aAlpha);
    for (ID i = 0; i < myMuContainersAvailable; i++) {
      myContainerToNodes.emplace_back(e);
    }
  }

  // filter the mu-apps only
  std::vector<ID> myMuApps;
  for (ID a = 0; a < theApps.size(); a++) {
    if (theApps[a].theType == Type::Mu) {
      myMuApps.emplace_back(a);
    }
  }
  assert(myMuApps.size() == myNumMu);

  // assignment problem input matrix, with costs from apps to containers
  hungarian::HungarianAlgorithm::DistMatrix myApMatrix(
      myMuApps.size(), std::vector<double>(myContainerToNodes.size()));
  for (ID a = 0; a < myMuApps.size(); a++) {
    for (ID c = 0; c < myContainerToNodes.size(); c++) {
      myApMatrix[a][c] =
          networkCost(theApps[myMuApps[a]].theBroker, myContainerToNodes[c]);
    }
  }
  VLOG(2) << "assignment problem cost matrix:\n"
          << uiiit::lambdamusim::toString(myApMatrix);

  // solve assignment problem
  std::vector<int> myMuAssignment;
  ret.theMuCost =
      hungarian::HungarianAlgorithm::Solve(myApMatrix, myMuAssignment);

  // assign apps to edges (and vice versa)
  for (ID i = 0; i < myMuAssignment.size(); i++) {
    const auto myAppId  = myMuApps[i];
    const auto myEdgeId = myContainerToNodes[myMuAssignment[i]];
    ret.theMuCloud += myEdgeId == CLOUD ? 1 : 0;
    theApps[myAppId].theEdge = myEdgeId;
    theEdges[myEdgeId].theMuApps.emplace_back(myAppId);
  }

  //
  // assign lambda-apps to the remaining containers, taking into account the
  // beta factor, app requests, and container capacities
  //

  // filter the lambda-apps only
  std::vector<ID> myLambdaApps;
  for (ID a = 0; a < theApps.size(); a++) {
    if (theApps[a].theType == Type::Lambda) {
      myLambdaApps.emplace_back(a);
    }
  }
  assert(myLambdaApps.size() == myNumLambda);

  // filter the containers not assigned to mu-apps
  std::vector<ID> myLambdaContainers;
  for (ID e = 0; e < theEdges.size(); e++) {
    assert(theEdges[e].theNumContainers >= theEdges[e].theMuApps.size());
    const auto myNumContainers =
        theEdges[e].theNumContainers - theEdges[e].theMuApps.size();
    for (std::size_t i = 0; i < myNumContainers; i++) {
      myLambdaContainers.emplace_back(e);
    }
  }

  // fill the request vector
  Mcfp::Requests myLambdaRequests(myLambdaApps.size(), aLambdaRequest);

  // fill the capacities vector
  Mcfp::Capacities myLambdaCapacities;
  for (const auto& e : myLambdaContainers) {
    myLambdaCapacities.emplace_back(
        static_cast<long>(aBeta * theEdges[e].theContainerCapacity));
  }

  // fill the cost matrix
  Mcfp::Costs myLambdaCosts(myLambdaApps.size(),
                            std::vector<double>(myLambdaContainers.size()));
  for (ID a = 0; a < myLambdaApps.size(); a++) {
    for (ID e = 0; e < myLambdaContainers.size(); e++) {
      myLambdaCosts[a][e] =
          networkCost(theApps[a].theBroker, myLambdaContainers[e]);
    }
  }

  // solve the minimum cost flow problem
  ret.theLambdaCost =
      Mcfp::solve(myLambdaCosts, myLambdaRequests, myLambdaCapacities);

  VLOG(2) << "apps after assignment:\n" << appsToString();

  return ret;
}

double& Scenario::networkCost(const ID aBroker, const ID aEdge) noexcept {
  return const_cast<double&>(
      const_cast<const Scenario*>(this)->networkCost(aBroker, aEdge));
}

const double& Scenario::networkCost(const ID aBroker,
                                    const ID aEdge) const noexcept {
  const auto myIndex = aBroker * theEdges.size() + aEdge;
  assert(myIndex < theNetworkCost.size());
  return theNetworkCost[myIndex];
}

std::string Scenario::networkCostToString() const {
  std::stringstream ret;
  for (ID b = 0; b < theBrokers.size(); b++) {
    ret << '[' << b << ']';
    for (ID e = 0; e < theEdges.size(); e++) {
      ret << ' ' << networkCost(b, e);
    }
    ret << '\n';
  }
  return ret.str();
}

std::string Scenario::appsToString() const {
  std::stringstream ret;
  for (ID a = 0; a < theApps.size(); a++) {
    ret << "app#" << a << " - broker#" << theApps[a].theBroker << " ["
        << toString(theApps[a].theType) << "]";
    if (theApps[a].theType == Type::Mu) {
      ret << " -> " << theApps[a].theEdge;
    }
    ret << '\n';
  }
  return ret.str();
}

std::string Scenario::toString(const Type aType) {
  switch (aType) {
    case Type::Lambda:
      return "lambda";
    case Type::Mu:
      return "mu";
    default:
      return "unknown";
  }
  assert(false);
}

} // namespace lambdamusim
} // namespace uiiit
