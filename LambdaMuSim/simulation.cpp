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

#include "Dataset/afdb-utils.h"
#include "LambdaMuSim/appperiods.h"
#include "LambdaMuSim/scenario.h"
#include "StateSim/network.h"
#include "StateSim/node.h"
#include "Support/tostring.h"

#include <glog/logging.h>

#include <algorithm>
#include <cmath>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>

namespace uiiit {
namespace lambdamusim {

std::string Desc::toString() const {
  std::stringstream myStream;
  myStream << "seed=" << theSeed;
  return myStream.str();
}

std::vector<std::string> Conf::toStrings() const {
  return std::vector<std::string>({
      std::to_string(theCloudDistanceFactor),
      std::to_string(myCloudStorageCost),
      std::to_string(theEpoch),
      std::to_string(theMinPeriods),
      std::to_string(theAvgApps),
      std::to_string(theAvgLambda),
      std::to_string(theAvgMu),
      std::to_string(theAlpha),
      std::to_string(theBeta),
      theAppModel,
  });
}

const std::vector<std::string>& Conf::toColumns() {
  static const std::vector<std::string> ret({
      "cloud-distance-factor",
      "cloud-storage-cost",
      "epoch",
      "min-periods",
      "avg-apps",
      "avg-lambda",
      "avg-mu",
      "alpha",
      "beta",
      "app-model",
  });
  return ret;
}

std::string Conf::type() const {
  switch (theType) {
    case Type::Dynamic:
      return "dynamic";
    case Type::Snapshot:
      return "snapshot";
    default:
      return "unknown";
  }
  assert(false);
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
        PerformanceData myPerformanceData;
        if (myDesc.theConf->theType == Conf::Type::Snapshot) {
          myPerformanceData =
              myDesc.theScenario->snapshot(myDesc.theConf->theAvgLambda,
                                           myDesc.theConf->theAvgMu,
                                           myDesc.theConf->theAlpha,
                                           myDesc.theConf->theBeta,
                                           myDesc.theSeed);

        } else if (myDesc.theConf->theType == Conf::Type::Dynamic) {
          assert(myDesc.theAppPeriods != nullptr);
          myPerformanceData =
              myDesc.theScenario->dynamic(myDesc.theConf->theDuration,
                                          myDesc.theConf->theWarmUp,
                                          myDesc.theConf->theEpoch,
                                          *myDesc.theAppPeriods,
                                          myDesc.theConf->theAvgApps,
                                          myDesc.theConf->theAlpha,
                                          myDesc.theConf->theBeta,
                                          myDesc.theSeed);

        } else {
          throw std::runtime_error("Analysis type not implemented: " +
                                   myDesc.theConf->type());
        }

        {
          // add the performance data to the in-memory structure
          const std::lock_guard<std::mutex> myLock(theSimulation.theMutex);
          theSimulation.theData.emplace_back(myDesc.theSeed,
                                             std::move(myPerformanceData));
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
    : theMutex()
    , theNumThreads(aNumThreads)
    , theWorkers()
    , theQueueIn()
    , theDesc()
    , theData() {
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
            << " to seed " << (aStartingSeed + aNumReplications);

  // clean up previous output data
  theData.clear();

  // load network from files
  const auto myNetwork = std::make_shared<statesim::Network>(
      aConf.theNodesPath, aConf.theLinksPath, aConf.theEdgesPath);

  // create the apps' periods
  const auto myAppPeriods =
      aConf.theType == Conf::Type::Dynamic ?
          std::make_unique<const AppPeriods>(
              dataset::loadTimestampDataset(aConf.theAppsPath),
              dataset::CostModel{0, 0.6, 0.4, 0.5, 6.3e-6, 0, 12, 12},
              aConf.theMinPeriods) :
          nullptr;

  // create the scenarios
  theDesc.resize(aNumReplications);
  size_t myDescCounter = 0;
  for (size_t myRun = 0; myRun < aNumReplications; myRun++) {
    assert(myDescCounter < theDesc.size());
    auto& myDesc = theDesc[myDescCounter++];

    myDesc.theConf = &aConf;
    myDesc.theAppPeriods =
        myAppPeriods.get() != nullptr ? &myAppPeriods->periods() : nullptr;
    myDesc.theSeed     = myRun + aStartingSeed;
    myDesc.theAppModel = makeAppModel(myDesc.theSeed, aConf.theAppModel);
    assert(myDesc.theAppModel.get() != nullptr);
    myDesc.theScenario = std::make_unique<Scenario>(
        *myNetwork,
        myDesc.theConf->theCloudDistanceFactor,
        myDesc.theConf->myCloudStorageCost,
        [](const auto& aNode) {
          const auto myNumContainers =
              static_cast<std::size_t>(std::round(aNode.speed() / 1e9));
          if (myNumContainers == 0) {
            throw std::runtime_error("Invalid node (num containers): " +
                                     aNode.toString());
          }
          return myNumContainers;
        },
        [](const auto& aNode) {
          if (aNode.name().find("server") != std::string::npos) {
            return 20;
          } else if (aNode.name().find("nuc") != std::string::npos) {
            return 10;
          }
          throw std::runtime_error("Invalid node (container speed): " +
                                   aNode.toString());
        },
        *myDesc.theAppModel);
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

  // save simulation output
  save(aConf, theData);
}

void Simulation::save(const Conf& aConf, const Data& aData) {
  std::ofstream myOut(aConf.theOutfile,
                      aConf.theAppend ? std::ios::app : std::ios::trunc);
  if (not myOut) {
    throw std::runtime_error("could not open file for writing: " +
                             aConf.theOutfile);
  }
  for (const auto& myRecord : aData) {
    assert(aConf.toStrings().size() == Conf::toColumns().size());
    assert(std::get<1>(myRecord).toStrings().size() ==
           PerformanceData::toColumns().size());

    myOut << std::get<0>(myRecord) << ',' << ::toString(aConf.toStrings(), ",")
          << ',' << ::toString(std::get<1>(myRecord).toStrings(), ",") << '\n';
  }
}

} // namespace lambdamusim
} // namespace uiiit
