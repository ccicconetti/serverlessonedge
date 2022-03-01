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

#include "StateSim/network.h"
#include "StateSim/node.h"
#include "Support/random.h"

#include <glog/logging.h>

#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace uiiit {
namespace lambdamusim {

Scenario::Scenario(
    statesim::Network&                                       aNetwork,
    const std::function<std::size_t(const statesim::Node&)>& aNumContainers,
    const std::function<double(const statesim::Node&)>&      aContainerCapacity)
    : theApps()
    , theBrokers()
    , theEdges()
    , theNetworkCost() {

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
  const double myCloudDistance = 1.0 + myMaxDistance;
  for (ID b = 0; b < theBrokers.size(); b++) {
    networkCost(b, 0) = myCloudDistance;
  }
}

PerformanceData Scenario::snapshot(const std::size_t aAvgLambda,
                                   const std::size_t aAvgMu,
                                   const double      aAlpha,
                                   const double      aBeta,
                                   const double      aLambdaRequest,
                                   const std::size_t aSeed) {
  PerformanceData ret;

  support::UniformIntRv<ID> myBrokerRv(0, theBrokers.size() - 1, aSeed, 0, 0);
  const auto myNumLambda = support::PoissonRv(aAvgLambda, aSeed, 0, 0)();
  const auto myNumMu     = support::PoissonRv(aAvgMu, aSeed, 1, 0)();

  for (std::size_t i = 0; i < myNumLambda; i++) {
    theApps.emplace_back(myBrokerRv(), Type::Lambda);
    theBrokers[theApps.back().theBroker].theApps.emplace_back(i);
  }

  for (std::size_t i = 0; i < myNumMu; i++) {
    theApps.emplace_back(myBrokerRv(), Type::Mu);
    theBrokers[theApps.back().theBroker].theApps.emplace_back(i);
  }

  // set the cloud num containers and capacity
  theEdges[0].theNumContainers     = myNumLambda + myNumMu;
  theEdges[0].theContainerCapacity = myNumLambda * aLambdaRequest;

  VLOG(2) << "seed " << aSeed << ", " << myNumLambda << " lambda-apps, "
          << myNumMu << " mu-apps, network costs:\n"
          << networkCostToString();

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

} // namespace lambdamusim
} // namespace uiiit
