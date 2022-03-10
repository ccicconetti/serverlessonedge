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

#include "LambdaMuSim/apppool.h"
#include "LambdaMuSim/mcfp.h"
#include "StateSim/network.h"
#include "StateSim/node.h"
#include "Support/random.h"
#include "Support/tostring.h"
#include "hungarian-algorithm-cpp/Hungarian.h"

#include <boost/accumulators/framework/accumulator_set.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <glog/logging.h>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/weighted_mean.hpp>

#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace ba = boost::accumulators;

namespace uiiit {
namespace lambdamusim {

bool PerformanceData::operator==(const PerformanceData& aOther) const noexcept {
  return theNumLambda == aOther.theNumLambda and theNumMu == aOther.theNumMu and
         theNumContainers == aOther.theNumContainers and
         theTotCapacity == aOther.theTotCapacity and
         theLambdaCost == aOther.theLambdaCost and
         theMuCost == aOther.theMuCost and theMuCloud == aOther.theMuCloud and
         theMuMigrations == aOther.theMuMigrations and
         theNumOptimizations == aOther.theNumOptimizations;
}

std::vector<std::string> PerformanceData::toStrings() const {
  return std::vector<std::string>({
      std::to_string(theNumContainers),
      std::to_string(theTotCapacity),
      std::to_string(theNumLambda),
      std::to_string(theNumMu),
      std::to_string(theLambdaCost),
      std::to_string(theMuCost),
      std::to_string(theMuCloud),
      std::to_string(theMuMigrations),
      std::to_string(theNumOptimizations),
  });
}

const std::vector<std::string>& PerformanceData::toColumns() {
  static const std::vector<std::string> ret({"num-containers",
                                             "tot-capacity",
                                             "num-lambda",
                                             "num-mu",
                                             "lambda-cost",
                                             "mu-cost",
                                             "mu-cloud",
                                             "mu-migrations",
                                             "num-optimizations"});
  return ret;
}

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

PerformanceData
Scenario::dynamic(const double                           aDuration,
                  const double                           aWarmUp,
                  const double                           aEpoch,
                  const std::vector<std::deque<double>>& aPeriods,
                  const std::size_t                      aAvgApps,
                  const double                           aAlpha,
                  const double                           aBeta,
                  const long                             aLambdaRequest,
                  const std::size_t                      aSeed) {
  struct Accumulator {
    Accumulator(const double& aClock, const double aWarmUp)
        : theClock(aClock)
        , theWarmUp(aWarmUp)
        , theLast(0)
        , theAcc() {
      // noop
    }
    void operator()(const double aValue) {
      if (theClock >= theWarmUp) {
        theAcc(aValue, ba::weight = (theClock - theLast));
        theLast = theClock;
      }
    }
    double mean() {
      return ba::weighted_mean(theAcc);
    }
    const double& theClock;
    const double  theWarmUp;
    double        theLast;
    ba::accumulator_set<double, ba::stats<ba::tag::weighted_mean>, double>
        theAcc;
  };

  checkArgs(aAlpha, aBeta, aLambdaRequest);
  if (aDuration < 0) {
    throw std::runtime_error("Invalid simulation duration, must be >= 0: " +
                             std::to_string(aDuration));
  }
  if (aWarmUp < 0) {
    throw std::runtime_error("Invalid warm-up duration, must be >= 0: " +
                             std::to_string(aWarmUp));
  }

  PerformanceData ret;

  support::UniformIntRv<ID> myBrokerRv(0, theBrokers.size() - 1, aSeed, 0, 0);
  const auto myNumApps = support::PoissonRv(aAvgApps, aSeed, 0, 0)();

  // clear apps and brokers and any previous assignment between edge nodes
  // and apps from previous calls
  clearPreviousAssignments(ret);

  // initialize apps and brokers, all apps are born in a lambda state
  for (std::size_t i = 0; i < myNumApps; i++) {
    theApps.emplace_back(myBrokerRv(), Type::Lambda);
    theBrokers[theApps.back().theBroker].theApps.emplace_back(i);
  }

  // set the cloud num containers and capacity
  theEdges[CLOUD].theNumContainers     = 1 + myNumApps;
  theEdges[CLOUD].theContainerCapacity = myNumApps * aLambdaRequest / aBeta;
  ret.theNumContainers += theEdges[CLOUD].theNumContainers;
  ret.theTotCapacity += theEdges[CLOUD].theContainerCapacity;

  VLOG(2) << "seed " << aSeed << ", " << myNumApps << " apps, network costs:\n"
          << networkCostToString();

  AppPool     myAppPool(aPeriods, myNumApps, aSeed);
  double      myClock      = 0;
  double      myNextEpoch  = 0;
  std::size_t myNumLambda  = myNumApps; // all apps start as lambda
  double      myLambdaCost = 0;
  double      myMuCost     = 0;
  Accumulator myNumLambdaAcc(myClock, aWarmUp);
  Accumulator myNumMuAcc(myClock, aWarmUp);
  Accumulator myLambdaCostAcc(myClock, aWarmUp);
  Accumulator myMuCostAcc(myClock, aWarmUp);
  Accumulator myMuCloudAcc(myClock, aWarmUp);
  auto        myOptimize = true; // first time or when apps change
  while (myClock < aDuration) {
    assert(myNextEpoch >= 0);
    assert(myAppPool.next() >= 0);

    double myTimeElapsed = 0;
    if (myNextEpoch <= myAppPool.next()) {
      // perform periodic optimization
      VLOG(2) << (myClock + myNextEpoch) << " a new epoch begins "
              << (myOptimize ? "(optimize)" : " (skip)");

      myAppPool.advance(myNextEpoch);
      myTimeElapsed = myNextEpoch;
      myNextEpoch   = aEpoch;

      // if there have not been any changes since last epoch, do nothing
      if (not myOptimize) {
        myClock += myTimeElapsed;
        continue;
      }

      if (myClock >= aWarmUp) {
        ret.theNumOptimizations++;
      }

      // save the old mu-app assignment (arbitrary on first epoch)
      std::map<ID, ID> myOldAssignment; // app id -> edge id
      for (ID a = 0; a < theApps.size(); a++) {
        if (theApps[a].theType == Type::Mu) {
          [[maybe_unused]] const auto myRet =
              myOldAssignment.emplace(a, theApps[a].theEdge);
          assert(myRet.second == true);
        }
      }

      PerformanceData myData;

      // assign mu-apps to containers, taking into account the alpha factor
      // the capacities (and beta factor) are ignored
      assignMuApps(aAlpha, myData);
      myMuCost = myData.theMuCost;
      myMuCloudAcc(myData.theMuCloud);

      // count the number of mu-app migrations occurring
      for (const auto& elem : myOldAssignment) {
        const auto a = elem.first;  // app id
        const auto e = elem.second; // edge node id
        if (theApps[a].theEdge != e) {
          if (myClock >= aWarmUp) {
            ret.theMuMigrations++;
          }
        }
      }

      // assign lambda-apps to the remaining containers, taking into account the
      // beta factor, app requests, and container capacities
      assignLambdaApps(aBeta, aLambdaRequest, myData);
      myLambdaCost = myData.theLambdaCost;

      // the flag will be set to true if there are app changes before next epoch
      myOptimize = false;

      VLOG(2) << "apps after assignment:\n" << appsToString();

    } else {
      // perform in-between-epochs assignment

      std::size_t myChanged;
      std::tie(myChanged, myTimeElapsed) = myAppPool.advance();
      myNextEpoch -= myTimeElapsed;
      auto&                       myType    = theApps[myChanged].theType;
      [[maybe_unused]] const auto myNewType = flip(myType);
      myOptimize                            = true;

      // migrate the changed app only
      double myOldCost;
      double myNewCost;
      if (myType == Type::Lambda) { // lambda -> mu
        assert(myNumLambda > 0);
        assert(myNumApps >= myNumLambda);

        std::tie(myOldCost, myNewCost) = migrateLambdaToMu(myChanged, aAlpha);

        myLambdaCost -= myOldCost;
        myMuCost += myNewCost;
        myNumLambda--;

        VLOG(2) << myClock << " app#" << myChanged
                << " changed from lambda to mu: cost lambda -" << myOldCost
                << ", mu +" << myNewCost << " (there are " << myNumLambda << "|"
                << (myNumApps - myNumLambda) << " lambda|mu apps)";

      } else { // mu -> lambda
        assert(myType == Type::Mu);
        assert(myNumLambda < myNumApps);
        assert(myNumApps >= myNumLambda);

        std::tie(myOldCost, myNewCost) =
            migrateMuToLambda(myChanged, aLambdaRequest);

        myLambdaCost += myNewCost;
        myMuCost -= myOldCost;
        myNumLambda++;

        VLOG(2) << myClock << " app#" << myChanged
                << " changed from mu to lambda: cost lambda +" << myNewCost
                << ", mu -" << myOldCost << " (there are " << myNumLambda << "|"
                << (myNumApps - myNumLambda) << " lambda|mu apps)";
      }
      assert(myType == myNewType);
      myNumLambdaAcc(myNumLambda);
      myNumMuAcc(myNumApps - myNumLambda);
    }
    myLambdaCostAcc(myLambdaCost);
    myMuCostAcc(myMuCost);

    assert(myTimeElapsed >= 0);
    myClock += myTimeElapsed;
  }

  // save weighted means to the output
  ret.theNumLambda  = myNumLambdaAcc.mean();
  ret.theNumMu      = myNumMuAcc.mean();
  ret.theLambdaCost = myLambdaCostAcc.mean();
  ret.theMuCost     = myMuCostAcc.mean();
  ret.theMuCloud    = myMuCloudAcc.mean();

  return ret;
}

PerformanceData Scenario::snapshot(const std::size_t aAvgLambda,
                                   const std::size_t aAvgMu,
                                   const double      aAlpha,
                                   const double      aBeta,
                                   const long        aLambdaRequest,
                                   const std::size_t aSeed) {
  checkArgs(aAlpha, aBeta, aLambdaRequest);

  PerformanceData ret;

  support::UniformIntRv<ID> myBrokerRv(0, theBrokers.size() - 1, aSeed, 0, 0);
  const auto myNumLambda = support::PoissonRv(aAvgLambda, aSeed, 0, 0)();
  const auto myNumMu     = support::PoissonRv(aAvgMu, aSeed, 1, 0)();

  // clear apps and brokers and any previous assignment between edge nodes
  // and apps from previous calls
  clearPreviousAssignments(ret);

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
  theEdges[CLOUD].theContainerCapacity = myNumLambda * aLambdaRequest / aBeta;
  ret.theNumContainers += theEdges[CLOUD].theNumContainers;
  ret.theTotCapacity += theEdges[CLOUD].theContainerCapacity;

  VLOG(2) << "seed " << aSeed << ", " << myNumLambda << " lambda-apps, "
          << myNumMu << " mu-apps, network costs:\n"
          << networkCostToString();

  // save assignment-independent output
  ret.theNumLambda = myNumLambda;
  ret.theNumMu     = myNumMu;

  // assign mu-apps to containers, taking into account the alpha factor
  // the capacities (and beta factor) are ignored
  assignMuApps(aAlpha, ret);

  // assign lambda-apps to the remaining containers, taking into account the
  // beta factor, app requests, and container capacities
  assignLambdaApps(aBeta, aLambdaRequest, ret);

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
    } else {
      ret << ", weights {"
          << ::toString(theApps[a].theWeights,
                        ",",
                        [](const auto& pair) {
                          return "(" + std::to_string(pair.first) + "," +
                                 std::to_string(pair.second) + ")";
                        })
          << "}";
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

void Scenario::assignMuApps(const double aAlpha, PerformanceData& aData) {
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

  // no mu-apps to assign
  if (myMuApps.empty()) {
    return;
  }

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
  aData.theMuCost =
      hungarian::HungarianAlgorithm::Solve(myApMatrix, myMuAssignment);

  // clear previous assignments
  for (auto& myEdge : theEdges) {
    myEdge.theMuApps.clear();
  }

  // assign apps to edges (and vice versa)
  for (ID i = 0; i < myMuAssignment.size(); i++) {
    const auto myAppId  = myMuApps[i];
    const auto myEdgeId = myContainerToNodes[myMuAssignment[i]];
    aData.theMuCloud += myEdgeId == CLOUD ? 1 : 0;
    theApps[myAppId].theEdge = myEdgeId;
    theEdges[myEdgeId].theMuApps.emplace_back(myAppId);
  }
}

void Scenario::assignLambdaApps(const double     aBeta,
                                const long       aLambdaRequest,
                                PerformanceData& aData) {
  // filter the lambda-apps only
  std::vector<ID> myLambdaApps;
  for (ID a = 0; a < theApps.size(); a++) {
    if (theApps[a].theType == Type::Lambda) {
      myLambdaApps.emplace_back(a);
    }
  }

  // no lambda-apps to assign
  if (myLambdaApps.empty()) {
    return;
  }

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
  VLOG(2) << "requests: ["
          << ::toString(
                 myLambdaRequests,
                 ",",
                 [](const auto& aValue) { return std::to_string(aValue); })
          << "]";

  // fill the capacities vector
  Mcfp::Capacities myLambdaCapacities;
  for (const auto& e : myLambdaContainers) {
    myLambdaCapacities.emplace_back(static_cast<long>(
        std::floor(aBeta * theEdges[e].theContainerCapacity)));
  }
  VLOG(2) << "capacities: ["
          << ::toString(
                 myLambdaCapacities,
                 ",",
                 [](const auto& aValue) { return std::to_string(aValue); })
          << "]";

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
  Mcfp::Weights myWeights;
  aData.theLambdaCost = Mcfp::solve(
      myLambdaCosts, myLambdaRequests, myLambdaCapacities, myWeights);

  assert(myWeights.size() == myLambdaApps.size());
  assert(not myWeights.empty());
  assert(myWeights[0].size() == myLambdaContainers.size());

  // copy the MCFP output weights (per container) into the apps' weights (per
  // edge node), by aggregating multiple containers per edge node by sum
  for (std::size_t t = 0; t < myWeights.size(); t++) {
    std::map<ID, double> myWeightMap;
    for (std::size_t w = 0; w < myWeights[t].size(); w++) {
      const auto myWeight = myWeights[t][w];
      const auto e        = myLambdaContainers[w]; // edge node index
      auto       it       = myWeightMap.emplace(e, 0);
      it.first->second += myWeight;
    }
    const auto a = myLambdaApps[t]; // app index
    theApps[a].theWeights.clear();
    for (const auto& elem : myWeightMap) {
      theApps[a].theWeights.emplace_back(elem.first, elem.second);
    }
  }
}

std::pair<double, double> Scenario::migrateLambdaToMu(const ID     aApp,
                                                      const double aAlpha) {
  assert(aApp < theApps.size());
  assert(theApps[aApp].theType == Type::Lambda);
  assert(not theApps[aApp].theWeights.empty());

  const auto myOldCost = lambdaCost(aApp);

  // find the least-cost free container
  ID     myCandidateEdge = std::numeric_limits<ID>::max();
  double myCandidateCost = std::numeric_limits<double>::max();
  for (ID e = 0; e < theEdges.size(); e++) {
    const auto myUsableMuContainers =
        e == CLOUD ?
            theEdges[e].theNumContainers :
            static_cast<std::size_t>(theEdges[e].theNumContainers * aAlpha);
    assert(myUsableMuContainers >= theEdges[e].theMuApps.size());
    const auto myAvailableMuContainers =
        myUsableMuContainers - theEdges[e].theMuApps.size();
    if (myAvailableMuContainers > 0) {
      const auto myCurrentCost = networkCost(theApps[aApp].theBroker, e);
      if (myCurrentCost < myCandidateCost) {
        myCandidateCost = myCurrentCost;
        myCandidateEdge = e;
      }
    }
  }
  assert(myCandidateEdge < theEdges.size());

  theEdges[myCandidateEdge].theMuApps.emplace_back(aApp);

  theApps[aApp].theEdge = myCandidateEdge;
  theApps[aApp].theWeights.clear();
  theApps[aApp].theType = Type::Mu;

  return {myOldCost, muCost(aApp)};
}

std::pair<double, double>
Scenario::migrateMuToLambda(const ID aApp, const long aLambdaRequest) {
  assert(aApp < theApps.size());
  assert(theApps[aApp].theType == Type::Mu);
  assert(theApps[aApp].theWeights.empty());

  // remove aApp from the list of applications served by its assigned edge
  const auto      myOldCost  = muCost(aApp);
  auto&           myOriginal = theEdges[theApps[aApp].theEdge].theMuApps;
  std::vector<ID> myCopy;
  for (const auto& a : myOriginal) {
    if (a != aApp) {
      myCopy.emplace_back(a);
    }
  }
  assert(myCopy.size() == (myOriginal.size() - 1));
  myCopy.swap(myOriginal);

  // migrate lambda to the cloud
  // theApps[aApp].theEdge is not meaningful anymore
  theApps[aApp].theWeights.emplace_back(CLOUD, aLambdaRequest);
  theApps[aApp].theType = Type::Lambda;

  return {myOldCost, lambdaCost(aApp)};
}

double Scenario::lambdaCost(const ID aApp) const {
  assert(aApp < theApps.size());
  assert(theApps[aApp].theType == Type::Lambda);
  assert(not theApps[aApp].theWeights.empty());

  double ret = 0;
  for (const auto& elem : theApps[aApp].theWeights) {
    ret += networkCost(theApps[aApp].theBroker, elem.first) * elem.second;
  }
  return ret;
}

double Scenario::muCost(const ID aApp) const {
  assert(aApp < theApps.size());
  assert(theApps[aApp].theType == Type::Mu);
  assert(theApps[aApp].theWeights.empty());

  return networkCost(theApps[aApp].theBroker, theApps[aApp].theEdge);
}

void Scenario::checkArgs(const double aAlpha,
                         const double aBeta,
                         const long   aLambdaRequest) {
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
}

Scenario::Type Scenario::flip(const Type aType) noexcept {
  switch (aType) {
    case Type::Lambda:
      return Type::Mu;
    case Type::Mu:
      return Type::Lambda;
    default:
      assert(false);
      return Type::Lambda;
  }
  assert(false);
}

void Scenario::clearPreviousAssignments(PerformanceData& aData) {
  theApps.clear();
  for (auto& myBroker : theBrokers) {
    myBroker.theApps.clear();
  }
  for (ID e = 0; e < theEdges.size(); e++) {
    aData.theNumContainers += e == CLOUD ? 0 : theEdges[e].theNumContainers;
    aData.theTotCapacity += e == CLOUD ? 0 : theEdges[e].theContainerCapacity;
    theEdges[e].theMuApps.clear();
  }
}

} // namespace lambdamusim
} // namespace uiiit