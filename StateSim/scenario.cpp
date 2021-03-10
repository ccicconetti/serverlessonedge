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

#include "StateSim/scenario.h"

#include "Support/split.h"

#include <glog/logging.h>

#include <limits>
#include <stdexcept>

namespace uiiit {
namespace statesim {

PerformanceData::Job::Job()
    : Job(0, 0, 0, 0) {
  // noop
}

PerformanceData::Job::Job(const double aProcDelay,
                          const double aNetDelay,
                          const size_t aDataTransfer,
                          const size_t aChainSize)
    : theProcDelay(aProcDelay)
    , theNetDelay(aNetDelay)
    , theDataTransfer(aDataTransfer)
    , theChainSize(aChainSize) {
  // noop
}

void PerformanceData::Job::merge(const Job& aOther) noexcept {
  theProcDelay += aOther.theProcDelay;
  theNetDelay += aOther.theNetDelay;
  theDataTransfer += aOther.theDataTransfer;
}

std::string PerformanceData::Job::toString() const {
  std::stringstream ret;
  ret << theProcDelay << ' ' << theNetDelay << ' ' << theDataTransfer << ' '
      << theChainSize;
  return ret.str();
}

bool PerformanceData::Job::operator==(const Job& aOther) const noexcept {
  return theProcDelay == aOther.theProcDelay and
         theNetDelay == aOther.theNetDelay and
         theDataTransfer == aOther.theDataTransfer and
         theChainSize == aOther.theChainSize;
}

bool PerformanceData::operator==(const PerformanceData& aOther) const {
  return theJobData == aOther.theJobData and theLoad == aOther.theLoad;
}

void PerformanceData::save(std::ofstream& aOutput) const {
  // save version number
  aOutput.write(reinterpret_cast<const char*>(&theVersion), sizeof(theVersion));

  // save number of jobs
  const size_t J = numJobs();
  aOutput.write(reinterpret_cast<const char*>(&J), sizeof(J));

  // save per-job samples
  aOutput.write(reinterpret_cast<const char*>(theJobData.data()),
                sizeof(PerformanceData::Job) * J);

  // save number of nodes
  const size_t N = numNodes();
  aOutput.write(reinterpret_cast<const char*>(&N), sizeof(N));

  // save per-node samples
  aOutput.write(reinterpret_cast<const char*>(theLoad.data()),
                sizeof(size_t) * N);
}

PerformanceData PerformanceData::load(std::ifstream& aInput) {
  PerformanceData ret;
  // read version number, abort if wrong
  size_t myVersion;
  aInput.read(reinterpret_cast<char*>(&myVersion), sizeof(myVersion));
  if (myVersion != theVersion) {
    throw std::runtime_error("Wrong version number: expected " +
                             std::to_string(theVersion) + ", found " +
                             std::to_string(myVersion));
  }

  // read number of jobs
  size_t myNumJobs;
  aInput.read(reinterpret_cast<char*>(&myNumJobs), sizeof(myNumJobs));

  // read per-job samples
  ret.theJobData.resize(myNumJobs);
  aInput.read(reinterpret_cast<char*>(ret.theJobData.data()),
              sizeof(PerformanceData::Job) * myNumJobs);

  // read number of nodes
  size_t myNumNodes;
  aInput.read(reinterpret_cast<char*>(&myNumNodes), sizeof(myNumNodes));

  // read per-node samples
  ret.theLoad.resize(myNumNodes);
  aInput.read(reinterpret_cast<char*>(ret.theLoad.data()),
              sizeof(size_t) * myNumNodes);

  return ret;
}

Scenario::Scenario(const Conf& aConf)
    : theRng(aConf.theSeed)
    , theSeed(aConf.theSeed)
    , theAffinities(randomAffinities(
          aConf.theFuncWeights, aConf.theAffinityWeights, theRng))
    , theNetwork(std::make_shared<Network>(
          aConf.theNodesPath, aConf.theLinksPath, aConf.theEdgesPath))
    , theJobs(loadJobs(aConf.theTasksPath,
                       aConf.theOpsFactor,
                       aConf.theArgFactor,
                       aConf.theStateFactor,
                       aConf.theFuncWeights,
                       aConf.theSeed,
                       aConf.theStatefulOnly))
    , theClients(randomClients(theJobs.size(), *theNetwork, theRng))
    , theLoad()
    , theAllocation() {
  LOG(INFO) << "Created scenario seed " << theSeed;
}

Scenario::Scenario(const std::map<std::string, Affinity>& aAffinities,
                   const std::shared_ptr<Network>&        aNetwork,
                   const std::vector<Job>&                aJobs,
                   const size_t                           aSeed)
    : theRng(aSeed)
    , theSeed(aSeed)
    , theAffinities(aAffinities)
    , theNetwork(aNetwork)
    , theJobs(aJobs)
    , theClients(randomClients(theJobs.size(), *theNetwork, theRng))
    , theLoad()
    , theAllocation() {
  LOG(INFO) << "Created scenario seed " << theSeed;
}

void Scenario::allocateTasks(const AllocPolicy aPolicy) {
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
      size_t myInSize  = 0;
      size_t myOutSize = 0;
      if (aPolicy == AllocPolicy::ProcNet) {
        std::tie(myInSize, myOutSize) = allStatesArgSizes(myJob, myTask, true);
      } else {
        assert(aPolicy == AllocPolicy::ProcOnly);
      }

      // select the processing node with shortest execution time
      auto  myMinExecTime = 0.0;
      Node* myMinNode     = nullptr;
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

PerformanceData Scenario::performance(const ExecPolicy aPolicy) const {
  LOG(INFO) << "measuring performance using policy " << toString(aPolicy);

  if (theLoad.empty()) {
    throw std::runtime_error(
        "Cannot measure performance without an allocation");
  }

  PerformanceData ret;

  //
  // measure performance for each job
  //
  ret.theJobData.reserve(theJobs.size());
  for (const auto& myJob : theJobs) {
    // retrieve the client of this job
    assert(myJob.id() < theClients.size());
    const auto myClient = theClients[myJob.id()];

    // last known location of the state, only used with StateLocal policy
    std::vector<Node*> myStateLocation(myJob.stateSizes().size(), myClient);

    PerformanceData::Job myExecStat;
    myExecStat.theChainSize = myJob.tasks().size();
    Node* myPrevNode        = nullptr;
    for (auto it = myJob.tasks().cbegin(); it != myJob.tasks().cend(); ++it) {
      const auto& myTask = *it;

      // retrieve the node to which this task has been allocated
      assert(myJob.id() < theAllocation.size());
      assert(myTask.id() < theAllocation[myJob.id()].size());
      const auto myNode = theAllocation[myJob.id()][myTask.id()];

      // identify the next task in the job
      const auto  jt         = std::next(it);
      const Task* myNextTask = nullptr;
      if (jt != myJob.tasks().cend()) {
        myNextTask = &(*jt);
      }

      // argument and return value sizes (+ states, if any, depending on policy)
      size_t myInSize;
      size_t myOutSize;

      // retrieve execution statistics
      VLOG(2) << "job " << myJob.id() << " task " << myTask.id() << " client "
              << myClient->name() << " allocated to " << myNode->name()
              << ", policy " << toString(aPolicy);

      if (aPolicy == ExecPolicy::PureFaaS) {
        std::tie(myInSize, myOutSize) = allStatesArgSizes(myJob, myTask, true);
        myExecStat.merge(execStatsTwoWay(
            myTask.ops(), myInSize, myOutSize, *myClient, *myNode));

      } else if (aPolicy == ExecPolicy::StatePropagate) {
        std::tie(myInSize, myOutSize) = allStatesArgSizes(myJob, myTask, true);
        myExecStat.merge(
            execStatsOneWay(myTask.ops(),
                            myInSize,
                            myPrevNode == nullptr ? *myClient : *myPrevNode,
                            *myNode));
        if (myNextTask == nullptr) {
          myExecStat.merge(execStatsOneWay(0, myOutSize, *myNode, *myClient));
        }

      } else if (aPolicy == ExecPolicy::StateLocal) {
        // execution time + transfer of the argument from previous node
        // to the current one
        myExecStat.merge(
            execStatsOneWay(myTask.ops(),
                            myTask.size(),
                            myPrevNode == nullptr ? *myClient : *myPrevNode,
                            *myNode));

        // transfer of state from its last known location
        for (const auto myStateId : myTask.deps()) {
          assert(myStateId < myJob.stateSizes().size());
          assert(myStateId < myStateLocation.size());
          myExecStat.merge(execStatsOneWay(0,
                                           myJob.stateSizes()[myStateId],
                                           *myStateLocation[myStateId],
                                           *myNode));

          // update location of this state to the current node
          myStateLocation[myStateId] = myNode;
        }

        // last return value to client
        if (myNextTask == nullptr) {
          myExecStat.merge(
              execStatsOneWay(0, myJob.retSize(), *myNode, *myClient));
        }

      } else if (aPolicy == ExecPolicy::UnchainedExternal or
                 aPolicy == ExecPolicy::UnchainedInEdge or
                 aPolicy == ExecPolicy::UnchainedInFunction) {

        myExecStat.merge(execStatsTwoWay(
            myTask.ops(),
            myTask.size(),
            myNextTask == nullptr ? myJob.retSize() : myNextTask->size(),
            *myClient,
            *myNode));

        // add state retrieve/update, if needed
        const auto myStateSize  = stateSize(myJob, myTask, false);
        Node*      myRepository = nullptr;
        if (aPolicy == ExecPolicy::UnchainedExternal) {
          // repository is the cloud node
          // myRepository = theNetwork->cloud();
        } else if (aPolicy == ExecPolicy::UnchainedInEdge) {
          // repository is the central node in the network
          myRepository = theNetwork->central();
        } else {
          // repository is in the same processing node => no data transfer
          assert(aPolicy == ExecPolicy::UnchainedInFunction);
          assert(myRepository == nullptr);
        }

        if (myRepository != nullptr) {
          myExecStat.merge(execStatsTwoWay(
              0, myStateSize, myStateSize, *myClient, *myRepository));
        }

      } else if (aPolicy == ExecPolicy::UnchainedInClient) {
        std::tie(myInSize, myOutSize) = allStatesArgSizes(myJob, myTask, false);
        myExecStat.merge(execStatsTwoWay(
            myTask.ops(), myInSize, myOutSize, *myClient, *myNode));

      } else {
        LOG(FATAL) << "unknown execution policy: " << toString(aPolicy);
      }

      myPrevNode = myNode;
    }
    ret.theJobData.emplace_back(myExecStat);
  }

  //
  // record the load for each processing node
  //
  ret.theLoad.reserve(theNetwork->processing().size());
  for (const auto myNodePtr : theNetwork->processing()) {
    assert(myNodePtr->id() < theLoad.size());
    ret.theLoad.emplace_back(theLoad[myNodePtr->id()]);
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

PerformanceData::Job Scenario::execStatsTwoWay(const size_t aOps,
                                               const size_t aInSize,
                                               const size_t aOutSize,
                                               const Node&  aOrigin,
                                               const Node&  aTarget) const {
  assert(aOps == 0 or aTarget.id() < theLoad.size());
  assert(aOps == 0 or theLoad[aTarget.id()] > 0);

  const auto myProcTime =
      aOps == 0 ? 0 : (aOps / (aTarget.speed() / theLoad[aTarget.id()]));
  const auto myTxTime = theNetwork->txTime(aOrigin, aTarget, aInSize) +
                        theNetwork->txTime(aTarget, aOrigin, aOutSize);
  const auto myDataTransferred = theNetwork->hops(aOrigin, aTarget) * aInSize +
                                 theNetwork->hops(aTarget, aOrigin) * aOutSize;

  return PerformanceData::Job(myProcTime, myTxTime, myDataTransferred, 0);
}

PerformanceData::Job Scenario::execStatsOneWay(const size_t aOps,
                                               const size_t aSize,
                                               const Node&  aOrigin,
                                               const Node&  aTarget) const {
  assert(aOps == 0 or aTarget.id() < theLoad.size());
  assert(aOps == 0 or theLoad[aTarget.id()] > 0);

  const auto myProcTime =
      aOps == 0 ? 0 : (aOps / (aTarget.speed() / theLoad[aTarget.id()]));
  const auto myTxTime          = theNetwork->txTime(aOrigin, aTarget, aSize);
  const auto myDataTransferred = theNetwork->hops(aOrigin, aTarget) * aSize;

  return PerformanceData::Job(myProcTime, myTxTime, myDataTransferred, 0);
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

size_t
Scenario::stateSize(const Job& aJob, const Task& aTask, const bool aAll) {
  size_t myStateSize = 0;
  if (aAll) {
    // all the states of the job
    for (const auto mySize : aJob.stateSizes()) {
      myStateSize += mySize;
    }
  } else {
    // only the states on which aTask depends
    for (const auto myStateId : aTask.deps()) {
      assert(myStateId < aJob.stateSizes().size());
      myStateSize += aJob.stateSizes()[myStateId];
    }
  }
  return myStateSize;
}

std::pair<size_t, size_t> Scenario::allStatesArgSizes(const Job&  aJob,
                                                      const Task& aTask,
                                                      const bool  aAll) {
  // size of the states (if any)
  const auto myStateSize = stateSize(aJob, aTask, aAll);

  // size of the input argument and return value, including state size
  const auto myInSize = myStateSize + aTask.size();
  const auto myOutSize =
      myStateSize + (((aTask.id() + 1) == aJob.tasks().size()) ?
                         aJob.retSize() :
                         aJob.tasks()[aTask.id() + 1].size());

  return {myInSize, myOutSize};
}

std::string toString(const ExecPolicy aPolicy) {
  // clang-format off
  switch (aPolicy) {
    case ExecPolicy::PureFaaS:            return "PureFaaS";
    case ExecPolicy::StatePropagate:      return "StatePropagate";
    case ExecPolicy::StateLocal:          return "StateLocal";
    case ExecPolicy::UnchainedExternal:   return "UnchainedExternal";
    case ExecPolicy::UnchainedInEdge:     return "UnchainedInEdge";
    case ExecPolicy::UnchainedInFunction: return "UnchainedInFunction";
    case ExecPolicy::UnchainedInClient:   return "UnchainedInClient";
    default:                              assert(false);
  }
  // clang-format on

  return std::string();
}

ExecPolicy execPolicyFromString(const std::string& aValue) {
  // clang-format off
         if (aValue == "PureFaaS") {            return ExecPolicy::PureFaaS;
  } else if (aValue == "StatePropagate") {      return ExecPolicy::StatePropagate;
  } else if (aValue == "StateLocal") {          return ExecPolicy::StateLocal;
  } else if (aValue == "UnchainedExternal") {   return ExecPolicy::UnchainedExternal;
  } else if (aValue == "UnchainedInEdge") {     return ExecPolicy::UnchainedInEdge;
  } else if (aValue == "UnchainedInFunction") { return ExecPolicy::UnchainedInFunction;
  } else if (aValue == "UnchainedInClient") {   return ExecPolicy::UnchainedInClient;
  }
  // clang-format on
  throw std::runtime_error("Invalid execution policy: " + aValue);
}

std::set<ExecPolicy> execPoliciesFromList(const std::string& aValue) {
  const auto myPolicies = support::split<std::vector<std::string>>(aValue, ",");
  std::set<ExecPolicy> ret;
  for (const auto& myPolicy : myPolicies) {
    ret.insert(execPolicyFromString(myPolicy));
  }
  return ret;
}

const std::set<ExecPolicy>& allExecPolicies() {
  static const std::set<ExecPolicy> myAllPolicies({
      ExecPolicy::PureFaaS,
      ExecPolicy::StatePropagate,
      ExecPolicy::StateLocal,
      ExecPolicy::UnchainedExternal,
      ExecPolicy::UnchainedInEdge,
      ExecPolicy::UnchainedInFunction,
      ExecPolicy::UnchainedInClient,
  });
  return myAllPolicies;
}

std::string toString(const AllocPolicy aPolicy) {
  // clang-format off
  switch (aPolicy) {
    case AllocPolicy::ProcOnly: return "ProcOnly";
    case AllocPolicy::ProcNet:  return "ProcNet";
    default:                    assert(false);
  }
  // clang-format on

  return std::string();
}

AllocPolicy allocPolicyFromString(const std::string& aValue) {
  // clang-format off
         if (aValue == "ProcOnly") { return AllocPolicy::ProcOnly;
  } else if (aValue == "ProcNet")  { return AllocPolicy::ProcNet;
  }
  // clang-format on
  throw std::runtime_error("Invalid allocation policy: " + aValue);
}

std::set<AllocPolicy> allocPoliciesFromList(const std::string& aValue) {
  const auto myPolicies = support::split<std::vector<std::string>>(aValue, ",");
  std::set<AllocPolicy> ret;
  for (const auto& myPolicy : myPolicies) {
    ret.insert(allocPolicyFromString(myPolicy));
  }
  return ret;
}

const std::set<AllocPolicy>& allAllocPolicies() {
  static const std::set<AllocPolicy> myAllPolicies({
      AllocPolicy::ProcOnly,
      AllocPolicy::ProcNet,
  });
  return myAllPolicies;
}

} // namespace statesim
} // namespace uiiit
