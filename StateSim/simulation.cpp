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

#include "StateSim/simulation.h"

#include "StateSim/job.h"
#include "StateSim/network.h"
#include "StateSim/scenario.h"

#include <glog/logging.h>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace uiiit {
namespace statesim {

std::string Simulation::Desc::toString() const {
  std::stringstream myStream;
  myStream << "alloc=" << statesim::toString(theAllocationPolicy)
           << ".exec=" << statesim::toString(theExecutionPolicy)
           << ".seed=" << theScenario->seed();
  return myStream.str();
}

std::string Simulation::Conf::toString() const {
  std::stringstream ret;
  ret << "nodes " << theNodesPath << ", links " << theLinksPath << ", edges "
      << theEdgesPath << ", tasks " << theTasksPath << ", " << theNumFunctions
      << " lambda functions, " << theNumJobs << " jobs, ops factor "
      << theOpsFactor << ", mem factor " << theMemFactor;
  return ret.str();
}

Simulation::Worker::Worker(Simulation& aSimulation)
    : theSimulation(aSimulation) {
  // noop
}

void Simulation::Worker::operator()() {
  auto myTerminated = false;
  while (not myTerminated) {
    try {
      const auto myId = theSimulation.theQueueIn.pop();
      VLOG(1) << "executing sim#" << myId;

      assert(myId < theSimulation.theDesc.size());
      auto& myDesc = theSimulation.theDesc[myId];

      myDesc.theScenario->allocateTasks(myDesc.theAllocationPolicy);
      myDesc.thePerformanceData =
          myDesc.theScenario->performance(myDesc.theExecutionPolicy);

      theSimulation.theQueueOut.push(myId);

    } catch (const support::QueueClosed&) {
      myTerminated = true;
    }
  }
}

void Simulation::Worker::stop() {
}

Simulation::Simulation(const size_t aNumThreads)
    : theNumThreads(aNumThreads)
    , theWorkers()
    , theQueueIn()
    , theDesc() {
  LOG(INFO) << "Initialize simulation environment with " << theNumThreads
            << " threads";
  for (size_t i = 0; i < aNumThreads; i++) {
    theWorkers.add(Worker(*this));
  }
  theWorkers.start();
}

Simulation::~Simulation() {
  theQueueOut.close();
  theQueueIn.close();
  theWorkers.stop();
  theWorkers.wait();
}

void Simulation::run(const Conf&  aConf,
                     const size_t aStartingSeed,
                     const size_t aNumReplications) {
  if (aConf.theNumJobs == 0) {
    throw std::runtime_error("Invalid zero jobs");
  }

  if (aConf.theNumFunctions == 0) {
    throw std::runtime_error("Invalid zero lambda function");
  }

  LOG(INFO) << "starting a batch of simulation from seed " << aStartingSeed
            << " to seed " << (aStartingSeed + aNumReplications)
            << " conf files " << aConf.toString();

  // load network from files
  const auto myNetwork = std::make_shared<Network>(
      aConf.theNodesPath, aConf.theLinksPath, aConf.theEdgesPath);

  // determine relative proportion of affinities
  std::map<Affinity, double> myAffinityWeights;
  for (const auto myNode : myNetwork->processing()) {
    myAffinityWeights[myNode->affinity()] += myNode->speed();
  }

  // create lambda function names
  std::map<std::string, double> myFuncWeights;
  for (size_t i = 0; i < aConf.theNumFunctions; i++) {
    myFuncWeights.emplace("f" + std::to_string(i), 1.0);
  }

  // determine lambda function affinities based on the proportion of processing
  // capabilities in the network
  auto mySum = 0.0;
  for (const auto& elem : myAffinityWeights) {
    mySum += elem.second;
  }
  auto                            it = myFuncWeights.begin();
  std::map<std::string, Affinity> myAffinities;
  for (const auto& elem : myAffinityWeights) {
    const auto myNumFunctions = static_cast<size_t>(
        std::round(aConf.theNumFunctions * elem.second / mySum));
    for (size_t i = 0; i < myNumFunctions and it != myFuncWeights.end();
         ++it, i++) {
      myAffinities.emplace(it->first, elem.first);
    }
  }

  // load all jobs from file (with seed = 0)
  auto myJobs = loadJobs(aConf.theTasksPath,
                         aConf.theOpsFactor,
                         aConf.theMemFactor,
                         myFuncWeights,
                         0,
                         true);

  LOG_IF(WARNING, myJobs.size() < aConf.theNumJobs)
      << "job pool size (" << myJobs.size()
      << ") smaller than target number of jobs per replication ("
      << aConf.theNumJobs << ")";

  // create the scenarios
  theDesc.resize(aNumReplications * allPolicies().size());
  size_t myDescCounter = 0;
  for (const auto myPolicy : allPolicies()) {
    for (size_t myRun = 0; myRun < aNumReplications; myRun++) {
      assert(myDescCounter < theDesc.size());
      auto&      myDesc = theDesc[myDescCounter++];
      const auto mySeed = myRun + aStartingSeed;

      std::default_random_engine myRng(mySeed);
      myDesc.theScenario = std::make_unique<Scenario>(
          myAffinities,
          myNetwork,
          selectJobs(myJobs, aConf.theNumJobs, myRng),
          mySeed);
      myDesc.theAllocationPolicy = myPolicy;
      myDesc.theExecutionPolicy  = myPolicy;
    }
  }

  // dispatch the simulations
  for (size_t i = 0; i < theDesc.size(); i++) {
    theQueueIn.push(i);
  }

  // wait for all the simulations to terminate
  while (myDescCounter > 0) {
    theQueueOut.pop();
    myDescCounter--;
  }
  if (not aConf.theOutfile.empty()) {
    save(aConf.theOutfile);
  }
  if (not aConf.theOutdir.empty()) {
    saveDir(aConf.theOutdir);
  }
}

std::vector<Job> Simulation::selectJobs(const std::vector<Job>&     aJobs,
                                        const size_t                aNumJobs,
                                        std::default_random_engine& aRng) {
  std::vector<Job> ret;

  const auto myProb = static_cast<double>(aNumJobs) / aJobs.size();
  std::uniform_real_distribution<double> myRv(0.0, 1.0);
  size_t                                 myCounter = 0;
  for (const auto& myJob : aJobs) {
    if (myRv(aRng) < myProb) {
      ret.emplace_back(Job::clone(myJob, myCounter++));
    }
  }

  return ret;
}

void Simulation::save(const std::string& aOutfile) {
  std::ofstream myOutstream(aOutfile);
  for (const auto& myDesc : theDesc) {
    myDesc.thePerformanceData.save(myOutstream);
  }
}

void Simulation::saveDir(const boost::filesystem::path& aDir) {
  boost::filesystem::create_directories(aDir);
  for (const auto& myDesc : theDesc) {
    std::ofstream myOutstream((aDir / ("out-" + myDesc.toString())).string());
    const auto&   myPerf = myDesc.thePerformanceData;
    for (size_t i = 0; i < myPerf.size(); i++) {
      myOutstream << myPerf.theProcDelays[i] << ' ' << myPerf.theNetDelays[i]
                  << ' ' << myPerf.theDataTransfer[i] << '\n';
    }
  }
}

} // namespace statesim
} // namespace uiiit
