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

#include "computer.h"

#include "container.h"
#include "edgecontrollermessages.h"
#include "edgemessages.h"
#include "lambda.h"
#include "processor.h"

#include <glog/logging.h>

#include <cassert>
#include <cmath>

namespace uiiit {
namespace edge {

Computer::Computer(const std::string&  aName,
                   const Callback&     aCallback,
                   const UtilCallback& aUtilCallback)
    : theName(aName)
    , theCallback(aCallback)
    , theUtilCallback(aUtilCallback)
    , theMutex()
    , theCondition()
    , theUtilCondition()
    , theInitDone(false)
    , theTerminating(false)
    , theNewTask(false)
    , theNextId(0)
    , theChrono(false)
    , theDispatcher()
    , theUtilCollector()
    , theProcessors()
    , theContainerNames()
    , theContainers() {
  if (not aCallback) {
    throw std::runtime_error("Call of computer " + aName + " not callable");
  }
}

Computer::~Computer() {
  if (theInitDone) {
    theTerminating = true;
    theCondition.notify_one();
    assert(theDispatcher.joinable());
    theDispatcher.join();

    if (theUtilCallback) {
      theUtilCondition.notify_one();
      assert(theUtilCollector.joinable());
      theUtilCollector.join();
    }
  }
}

void Computer::addProcessor(const std::string&  aName,
                            const ProcessorType aType,
                            const float         aSpeed,
                            const size_t        aCores,
                            const uint64_t      aMem) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  throwIfInitDone();
  if (theProcessors.find(aName) != theProcessors.end()) {
    throw DupProcessorName(theName, aName);
  }

  // everything is fine: add this processor
  theProcessors[aName] =
      std::make_unique<Processor>(aName, aType, aSpeed, aCores, aMem);
}

void Computer::addContainer(const std::string& aName,
                            const std::string& aProcName,
                            const Lambda&      aLambda,
                            const size_t       aNumWorkers) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  throwIfInitDone();
  if (theContainerNames.find(aName) != theContainerNames.end()) {
    throw DupContainerName(theName, aProcName, aName);
  }
  if (theContainers.find(aLambda.name()) != theContainers.end()) {
    throw DupLambdaName(theName, aLambda.name());
  }
  const auto myIt = theProcessors.find(aProcName);
  if (myIt == theProcessors.end()) {
    throw NoProcessorFound(theName, aProcName);
  }

  // everything is fine: add this container
  theContainerNames.insert(aName);
  theContainers[aLambda.name()] =
      std::make_unique<Container>(aName, *myIt->second, aLambda, aNumWorkers);
}

uint64_t Computer::addTask(const LambdaRequest& aRequest) {
  const std::lock_guard<std::mutex> myLock(theMutex);

  const auto myIt = theContainers.find(aRequest.theName);
  if (myIt == theContainers.end()) {
    throw NoContainerFound(theName, aRequest.theName);
  }

  if (not theInitDone) {
    // no more configuration allowed
    theInitDone = true;

    // start the dispatcher
    assert(not theDispatcher.joinable());
    theDispatcher = std::thread([this]() { dispatcher(); });

    // start the utilization collector, if needed
    assert(not theUtilCollector.joinable());
    if (theUtilCallback) {
      theUtilCollector = std::thread([this]() { utilCollector(); });
    }

    LOG(INFO) << "computer " << theName << ": starting";
  }

  // assign an identifier to this task
  const auto myId = theNextId++;

  // pause execution of the computer
  pause();

  // add the new task to the container
  assert(myIt->second);
  myIt->second->push(aRequest, myId);
  theNewTask = true;
  theCondition.notify_one();

  // resume execution of the computer
  resume();

  return myId;
}

double Computer::simTask(const LambdaRequest&   aRequest,
                         std::array<double, 3>& aLastUtils) {
  const std::lock_guard<std::mutex> myLock(theMutex);

  const auto myIt = theContainers.find(aRequest.theName);
  if (myIt == theContainers.end()) {
    throw NoContainerFound(theName, aRequest.theName);
  }

  LOG_IF(WARNING, not theInitDone)
      << "requested simulation of a task on a computer not yet started";

  aLastUtils = myIt->second->lastUtils();

  return myIt->second->simulate(aRequest);
}

std::shared_ptr<ContainerList> Computer::containerList() const {
  std::shared_ptr<ContainerList> myRet(new ContainerList());
  for (const auto& myContainer : theContainers) {
    myRet->theContainers.push_back(
        ContainerList::Container{myContainer.second->name(),
                                 myContainer.second->processor().name(),
                                 myContainer.second->lambda().name()});
  }
  return myRet;
}

void Computer::printProcessors(std::ostream& aStream) const {
  const std::lock_guard<std::mutex> myLock(theMutex);
  for (const auto& myProcessor : theProcessors) {
    assert(myProcessor.second);
    aStream << "processor " << *myProcessor.second << '\n';
  }
}

void Computer::printContainers(std::ostream& aStream) const {
  const std::lock_guard<std::mutex> myLock(theMutex);
  for (const auto& myContainer : theContainers) {
    assert(myContainer.second);
    aStream << "container " << *myContainer.second << '\n';
  }
}

void Computer::throwIfInitDone() const {
  if (theInitDone) {
    throw InitDone();
  }
}

void Computer::pause() {
  if (theChrono) {
    // this is the time since the last pause, in s
    const auto myElapsed = theChrono.stop();

    // adjust the virtual clock on all processors:
    for (const auto& myPair : theContainers) {
      assert(myPair.second);
      myPair.second->advance(myElapsed);
    }
  }
}

void Computer::resume() {
  assert(not theChrono);
  if (someActive()) {
    theChrono.start();
  }
}

void Computer::dispatcher() {
  while (true) {
    std::unique_lock<std::mutex> myLock(theMutex);
    int64_t                      mySleepTime = 1000000000; // in ns
    for (const auto& myPair : theContainers) {
      assert(myPair.second);
      auto& myContainer = *myPair.second;
      if (myContainer.active() == 0) {
        continue;
      }
      mySleepTime =
          std::min(mySleepTime,
                   static_cast<int64_t>(round(myContainer.nearest() * 1e9)));
    }

    theCondition.wait_for(
        myLock, std::chrono::nanoseconds(mySleepTime), [this]() {
          return theNewTask or someReady() or theTerminating;
        });

    if (theTerminating) {
      break;
    }

    theNewTask = false;

    if (someActive()) {
      pause();
      dispatchCompletedTasks();
      resume();
    }
  }
}

void Computer::utilCollector() {
  assert(theUtilCallback);
  std::map<std::string, double> myUtil;
  while (true) {
    std::unique_lock<std::mutex> myLock(theMutex);
    theUtilCondition.wait_for(
        myLock, std::chrono::seconds(1), [this]() { return theTerminating; });
    if (theTerminating) {
      break;
    }
    for (const auto& myProcessor : theProcessors) {
      myUtil[myProcessor.first] = myProcessor.second->utilization();
    }
    theUtilCallback(myUtil);
  }
}

bool Computer::someActive() const {
  assert(theInitDone);
  for (const auto& myPair : theContainers) {
    assert(myPair.second);
    if (myPair.second->active() > 0) {
      return true;
    }
  }
  return false;
}

bool Computer::someReady() const {
  assert(theInitDone);
  for (const auto& myPair : theContainers) {
    assert(myPair.second);
    if (myPair.second->active() > 0 and myPair.second->nearest() == 0) {
      return true;
    }
  }
  return false;
}

void Computer::dispatchCompletedTasks() {
  for (const auto& myPair : theContainers) {
    assert(myPair.second);
    auto& myContainer = *myPair.second;
    while (myContainer.active() > 0 and myContainer.nearest() == 0) {
      auto myCompletedTask = myContainer.pop();
      theCallback(myCompletedTask.theId, myCompletedTask.theResp);
    }
  }
}

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                aStream,
                         const uiiit::edge::Computer& aComputer) {
  aStream << "computer " << aComputer.name() << '\n';
  aComputer.printProcessors(aStream);
  aComputer.printContainers(aStream);
  return aStream;
}
