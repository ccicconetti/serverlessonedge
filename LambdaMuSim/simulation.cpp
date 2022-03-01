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

#include "LambdaMuSim/simulation.h"

#include "LambdaMuSim/scenario.h"
#include "StateSim/network.h"
#include "StateSim/node.h"

#include <glog/logging.h>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace uiiit {
namespace lambdamusim {

std::string Desc::toString() const {
  std::stringstream myStream;
  myStream << "seed=" << theSeed;
  return myStream.str();
}

std::string Conf::toString() const {
  std::stringstream ret;
  ret << "nodes " << theNodesPath << ", links " << theLinksPath << ", edges "
      << theEdgesPath << ", apps " << theAppsPath << ", alpha " << theAlpha
      << ", beta " << theBeta;
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

      auto myExceptionThrow = true;
      try {
        if (myDesc.theConf->theType == Conf::Type::Snapshot) {
          myDesc.thePerformanceData =
              myDesc.theScenario->snapshot(myDesc.theConf->theAvgLambda,
                                           myDesc.theConf->theAvgMu,
                                           myDesc.theConf->theAlpha,
                                           myDesc.theConf->theBeta,
                                           myDesc.theConf->theLambdaRequest,
                                           myDesc.theSeed);

        } else {
          throw std::runtime_error("NOT IMPLEMENTED");
        }
        myExceptionThrow = false;
      } catch (const std::exception& aErr) {
        LOG(ERROR) << "exception caught (" << aErr.what()
                   << ") when running: " << myDesc.toString();
      } catch (...) {
        LOG(ERROR) << "unknown exception caught when running: "
                   << myDesc.toString();
      }
      theSimulation.theQueueOut.push(not myExceptionThrow);

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
  LOG(INFO) << "starting a batch of simulation from seed " << aStartingSeed
            << " to seed " << (aStartingSeed + aNumReplications)
            << " conf files " << aConf.toString();

  // load network from files
  const auto myNetwork = std::make_shared<statesim::Network>(
      aConf.theNodesPath, aConf.theLinksPath, aConf.theEdgesPath);

  // create the scenarios
  theDesc.resize(aNumReplications);
  size_t myDescCounter = 0;
  for (size_t myRun = 0; myRun < aNumReplications; myRun++) {
    assert(myDescCounter < theDesc.size());
    auto& myDesc = theDesc[myDescCounter++];

    myDesc.theConf     = &aConf;
    myDesc.theSeed     = myRun + aStartingSeed;
    myDesc.theScenario = std::make_unique<Scenario>(
        *myNetwork,
        [](const statesim::Node& aNode) { return aNode.memory() / 1000; },
        [](const statesim::Node& aNode) { return aNode.speed() / 1e9; });
  }

  // dispatch the simulations
  for (size_t i = 0; i < theDesc.size(); i++) {
    theQueueIn.push(i);
  }

  // wait for all the simulations to terminate
  while (myDescCounter > 0) {
    const auto myGood = theQueueOut.pop();
    if (not myGood) {
      LOG(ERROR) << "bailing out because of exceptions thrown";
      return;
    }
    myDescCounter--;
  }
  if (not aConf.theOutfile.empty()) {
    save(aConf.theOutfile);
  }
  if (not aConf.theOutdir.empty()) {
    saveDir(aConf.theOutdir);
  }
}

void Simulation::save([[maybe_unused]] const std::string& aOutfile) {
  // std::ofstream myOutstream(aOutfile);
  // for (const auto& myDesc : theDesc) {
  //   myDesc.thePerformanceData.save(myOutstream);
  // }
}

void Simulation::saveDir(const boost::filesystem::path& aDir) {
  boost::filesystem::create_directories(aDir);
  // for (const auto& myDesc : theDesc) {
  //   const auto& myPerf = myDesc.thePerformanceData;

  //   {
  //     std::ofstream myOutstream((aDir / ("job-" +
  //     myDesc.toString())).string()); for (size_t i = 0; i < myPerf.numJobs();
  //     i++) {
  //       myOutstream << myPerf.theJobData[i].toString() << '\n';
  //     }
  //   }

  //   {
  //     std::ofstream myOutstream(
  //         (aDir / ("node-" + myDesc.toString())).string());
  //     for (size_t i = 0; i < myPerf.numNodes(); i++) {
  //       myOutstream << myPerf.theLoad[i] << '\n';
  //     }
  //   }
  // }
}

} // namespace lambdamusim
} // namespace uiiit
