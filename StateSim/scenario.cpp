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

#include "StateSim/scenario.h"

#include <glog/logging.h>

#include <limits>
#include <stdexcept>

namespace uiiit {
namespace statesim {

Scenario::Scenario(const Conf& aConf)
    : theRng(aConf.theSeed)
    , theAffinities(randomAffinities(
          aConf.theFuncWeights, aConf.theAffinityWeights, theRng))
    , theNetwork(std::make_unique<Network>(
          aConf.theNodesPath, aConf.theLinksPath, aConf.theEdgesPath))
    , theJobs(loadJobs(aConf.theTasksPath,
                       aConf.theOpsFactor,
                       aConf.theMemFactor,
                       aConf.theFuncWeights,
                       aConf.theSeed,
                       aConf.theStatefulOnly))
    , theClients(randomClients(theJobs.size(), *theNetwork, theRng))
    , theLoad()
    , theAllocation() {
  // noop
}

Scenario::Scenario(const std::map<std::string, Affinity>& aAffinities,
                   std::unique_ptr<Network>&&             aNetwork,
                   const std::vector<Job>&                aJobs,
                   const size_t                           aSeed)
    : theRng(aSeed)
    , theAffinities(aAffinities)
    , theNetwork(std::move(aNetwork))
    , theJobs(aJobs)
    , theClients(randomClients(theJobs.size(), *theNetwork, theRng))
    , theLoad()
    , theAllocation() {
  // noop
}

void Scenario::allocateTasks(const Policy aPolicy) {
  LOG(INFO) << "allocating tasks using policy " << toString(aPolicy);

  // clear any previous allocation and resize data structures
  theLoad       = std::vector<size_t>(theNetwork->nodes().size(), 0);
  theAllocation = Allocation(theJobs.size());

  // allocate all tasks for each job
  for (const auto& myJobId : shuffleJobIds()) {
    assert(myJobId < theJobs.size());
    const auto& myJob = theJobs[myJobId];

    // retrieve the client of this job
    assert(myJobId < theClients.size());
    const auto myClient = theClients[myJobId];

    // make room for all the tasks in the allocation table
    assert(myJobId < theAllocation.size());
    theAllocation[myJobId].resize(myJob.tasks().size());

    // allocate all the tasks of the current jobs
    for (size_t myTaskId = 0; myTaskId < myJob.tasks().size(); myTaskId++) {
      const auto& myTask = myJob.tasks()[myTaskId];

      // retrieve HW affinity
      const auto myAffinityIt = theAffinities.find(myTask.func());
      if (myAffinityIt == theAffinities.end()) {
        throw std::runtime_error("Cannot allocate task '" + myTask.func() +
                                 "' on job " + std::to_string(myJob.id()) +
                                 ": no matching affinity configured");
      }
      const auto myAffinity = myAffinityIt->second;

      // retrieve argument and return value sizes (+ states, if any)
      size_t myInSize;
      size_t myOutSize;
      std::tie(myInSize, myOutSize) = sizes(myJob, myTask);

      // select the processing node with shortest execution time
      double myMinExecTime;
      Node*  myMinNode = nullptr;
      for (const auto& myNode : theNetwork->processing()) {
        if (myNode->affinity() != myAffinity) {
          continue;
        }
        const auto myExecPair =
            execTimeNew(myTask.ops(), myInSize, myOutSize, *myClient, *myNode);
        const auto myCurExectime = myExecPair.first + myExecPair.second;
        if (myMinNode == nullptr or myCurExectime < myMinExecTime) {
          myMinExecTime = myCurExectime;
          myMinNode     = myNode;
        }
      }
      if (myMinNode == nullptr) {
        throw std::runtime_error(
            "Allocation failed: could not find any suitable node for task '" +
            myTask.func() + "'");
      }

      VLOG(2) << "allocated task " << myJob.id() << ":" << myTaskId
              << " from client " << myClient->name() << " on node "
              << myMinNode->name() << " (exec time " << (myMinExecTime * 1000)
              << " ms)";

      // update allocation status and the load on the node selected
      theAllocation[myJob.id()][myTaskId] = myMinNode;
      assert(myMinNode->id() < theLoad.size());
      theLoad[myMinNode->id()]++;
    }
  }
}

PerformanceData Scenario::performance(const Policy aPolicy) const {
  LOG(INFO) << "measuring performance using policy " << toString(aPolicy);

  PerformanceData ret;
  ret.theProcDelays.reserve(theJobs.size());
  ret.theNetDelays.reserve(theJobs.size());
  ret.theDataTransfer.reserve(theJobs.size());

  if (theLoad.empty()) {
    throw std::runtime_error(
        "Cannot measure performance without an allocation");
  }

  // measure performance for each job
  for (const auto& myJob : theJobs) {
    // retrieve the client of this job
    assert(myJob.id() < theClients.size());
    const auto myClient = theClients[myJob.id()];

    double myProcDelay       = 0;
    double myNetDelay        = 0;
    size_t myDataTransferred = 0;
    for (const auto& myTask : myJob.tasks()) {
      // retrieve the node to which this task has been allocated
      assert(myJob.id() < theAllocation.size());
      assert(myTask.id() < theAllocation[myJob.id()].size());
      const auto myNode = theAllocation[myJob.id()][myTask.id()];

      // retrieve argument and return value sizes (+ states, if any)
      size_t myInSize;
      size_t myOutSize;
      std::tie(myInSize, myOutSize) = sizes(myJob, myTask);

      // retrieve execution time
      const auto myExecPair =
          execTime(myTask.ops(), myInSize, myOutSize, *myClient, *myNode);
      myProcDelay += myExecPair.first;
      myNetDelay += myExecPair.second;

      // compute the amount of data transferred based on the number of hops
      myDataTransferred +=
          theNetwork->hops(*myClient, *myNode) * (myInSize + myOutSize);
    }
    ret.theProcDelays.emplace_back(myProcDelay);
    ret.theNetDelays.emplace_back(myNetDelay);
    ret.theDataTransfer.emplace_back(myDataTransferred);
  }

  return ret;
}

std::pair<double, double> Scenario::execTimeNew(const size_t aOps,
                                                const size_t aInSize,
                                                const size_t aOutSize,
                                                const Node&  aClient,
                                                const Node&  aNode) {
  assert(aNode.id() < theLoad.size());
  const auto myProcTime = aOps / (aNode.speed() / (theLoad[aNode.id()] + 1));
  const auto myTxTime   = theNetwork->txTime(aClient, aNode, aInSize) +
                        theNetwork->txTime(aNode, aClient, aOutSize);

  return {myProcTime, myTxTime};
}

std::pair<double, double> Scenario::execTime(const size_t aOps,
                                             const size_t aInSize,
                                             const size_t aOutSize,
                                             const Node&  aClient,
                                             const Node&  aNode) const {
  assert(aNode.id() < theLoad.size());
  assert(theLoad[aNode.id()] > 0);
  const auto myProcTime = aOps / (aNode.speed() / theLoad[aNode.id()]);
  const auto myTxTime   = theNetwork->txTime(aClient, aNode, aInSize) +
                        theNetwork->txTime(aNode, aClient, aOutSize);

  return {myProcTime, myTxTime};
}

std::vector<size_t> Scenario::shuffleJobIds() {
  std::set<size_t> myIds;
  for (size_t i = 0; i < theJobs.size(); i++) {
    myIds.emplace(i);
  }

  std::vector<size_t>                    ret;
  std::uniform_real_distribution<double> myRv;
  while (not myIds.empty()) {
    const auto myRnd   = myRv(theRng);
    const auto myRndId = static_cast<size_t>(std::floor(myRnd * myIds.size()));
    assert(myRndId < myIds.size());
    auto it = myIds.begin();
    std::advance(it, myRndId);
    assert(it != myIds.end());
    ret.emplace_back(*it);
    myIds.erase(it);
  }

  assert(ret.size() == theJobs.size());
  return ret;
}

std::vector<Node*> Scenario::randomClients(const size_t                aNumJobs,
                                           const Network&              aNetwork,
                                           std::default_random_engine& aRng) {
  std::vector<Node*>                    ret(aNumJobs);
  std::uniform_int_distribution<size_t> myRv(0, aNetwork.clients().size() - 1);
  for (size_t i = 0; i < ret.size(); i++) {
    ret[i] = aNetwork.clients()[myRv(aRng)];
  }
  return ret;
}

std::pair<size_t, size_t> Scenario::sizes(const Job& aJob, const Task& aTask) {
  // size of the states (if any)
  size_t myStateSize = 0;
  for (const auto myStateId : aTask.deps()) {
    assert(myStateId < aJob.stateSizes().size());
    myStateSize += aJob.stateSizes()[myStateId];
  }

  // size of the input argument and return value
  const auto myInSize = myStateSize + aTask.size();
  const auto myOutSize =
      myStateSize + (((aTask.id() + 1) == aJob.tasks().size()) ?
                         aJob.retSize() :
                         aJob.tasks()[aTask.id() + 1].size());

  return {myInSize, myOutSize};
}

std::string toString(const Scenario::Policy aPolicy) {
  switch (aPolicy) {
    case Scenario::Policy::PureFaaS:
      return "PureFaaS";
  }

  assert(false);
  return std::string();
}

Scenario::Policy policyFromString(const std::string& aValue) {
  if (aValue == "PureFaaS") {
    return Scenario::Policy::PureFaaS;
  }
  throw std::runtime_error("Invalid scenario policy: " + aValue);
}

} // namespace statesim
} // namespace uiiit
