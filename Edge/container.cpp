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

#include "container.h"

#include "edgemessages.h"
#include "processor.h"

#include <glog/logging.h>

#include <cassert>
#include <numeric>

namespace uiiit {
namespace edge {

Task::Task(const uint64_t aId,
           const uint64_t aMemory,
           const uint64_t aResidualOps)
    : theId(aId)
    , theMemory(aMemory)
    , theResidualOps(aResidualOps)
    , theResp(nullptr) {
}

Container::Container(const std::string& aName,
                     Processor&         aProcessor,
                     const Lambda&      aLambda,
                     const size_t       aNumWorkers)
    : theName(aName)
    , theProcessor(aProcessor)
    , theLambda(aLambda)
    , theNumWorkers(aNumWorkers)
    , theActive()
    , thePending() {
  if (aNumWorkers == 0) {
    throw std::runtime_error("Zero workers used for container " + aName);
  }
}

void Container::push(const LambdaRequest& aReq, uint64_t aId) {
  const auto myRequirements = requirements(aReq);
  Task myTask(aId, myRequirements.theMemory, myRequirements.theOperations);
  myTask.theResp = theLambda.execute(aReq, theProcessor.lastUtils());

  // if there are no spare workers or there is no spare memory available then
  // the current task becomes pending
  if (theActive.size() == theNumWorkers or
      theProcessor.memAvailable() < myRequirements.theMemory) {
    thePending.emplace_back(std::move(myTask));

  } else {
    // the task becomes active
    // 1. allocate memory
    // 2. add as active
    theProcessor.allocate(myRequirements.theMemory);
    makeActive(std::move(myTask));
  }
}

double Container::simulate(const LambdaRequest& aReq) const {
  // find requirements of the new lambda function
  const auto myRequirements = requirements(aReq);

  // if all the workers are busy then the equivalent speed of the processor will
  // be the same as it is now, otherwise we need to take into account that the
  // new task may require further resources on the processor for its execution
  const auto myOneMoreTask = theActive.size() < theNumWorkers;

  // simulate the dispatch of all the pending tasks
  auto myElapsed = // return value, in seconds
      std::accumulate(thePending.cbegin(),
                      thePending.cend(),
                      0.0,
                      [this](const double sum, const Task& aTask) {
                        return sum +
                               theProcessor.opsToTime(aTask.theResidualOps);
                      });

  // if all the workers are busy, then dispatch the one that will finish first
  if (theActive.size() == theNumWorkers) {
    myElapsed += theProcessor.opsToTime(theActive.front().theResidualOps);
  }

  // the new task can be put into (simulated) execution
  myElapsed += myOneMoreTask ?
                   theProcessor.opsToTimePlusOne(myRequirements.theOperations) :
                   theProcessor.opsToTime(myRequirements.theOperations);

  assert(myElapsed > 0);
  return myElapsed;
}

std::array<double, 3> Container::lastUtils() const noexcept {
  return theProcessor.lastUtils();
}

Task Container::pop() {
  throwIfEmpty();

  // remove the nearest-to-completion task from the active list
  auto myRet = theActive.front();
  theActive.pop_front();

  // free the memory of the nearest-to-completion task
  theProcessor.free(myRet.theMemory);

  // add as many pending tasks as possible provided that
  // 1. there are workers available in the container
  // 2. there is sufficient memory available on the processor
  for (auto myIt = thePending.begin(); myIt != thePending.end();) {
    if (theActive.size() == theNumWorkers or
        theProcessor.memAvailable() < myIt->theMemory) {
      break;
    }

    auto myTask = *myIt;
    theProcessor.allocate(myTask.theMemory);
    makeActive(std::move(myTask));

    myIt = thePending.erase(myIt);
  }

  return myRet;
}

void Container::advance(const double aElapsed) {
  if (aElapsed < 0) {
    throw std::runtime_error("cannot advance a container in the past by " +
                             std::to_string(-aElapsed) + " s");
  }

  if (theActive.empty()) {
    return;
  }

  const auto myOperations = theProcessor.timeToOps(aElapsed);
  const auto myActualOperations =
      std::min(theActive.front().theResidualOps, myOperations);
  VLOG_IF(1, myActualOperations != myOperations)
      << "Could not run " << myOperations << " operations on container "
      << theName << " hosted by processor " << theProcessor.name()
      << ", advancing by " << myActualOperations << " instead";
  VLOG(2) << theProcessor << ' ' << ", elapsed " << aElapsed << ", nearest "
          << nearest() << ", operations " << myOperations << '/'
          << myActualOperations;

  if (myActualOperations == 0) {
    return;
  }

  assert(myActualOperations <= theActive.front().theResidualOps);

  theActive.front().theResidualOps -= myActualOperations;
}

double Container::nearest() const {
  throwIfEmpty();
  return theProcessor.opsToTime(theActive.front().theResidualOps); // in s
}

void Container::throwIfEmpty() const {
  if (theActive.empty()) {
    throw std::runtime_error("No active tasks");
  }
}

void Container::makeActive(Task&& aTask) {
  assert(theActive.size() < theNumWorkers);

  // add the task to the active list, which is kept updated with only the
  // differences between a task and its subsequent elements
  auto     myIt  = theActive.begin();
  uint64_t mySum = 0;
  for (; myIt != theActive.end(); ++myIt) {
    if (mySum + myIt->theResidualOps > aTask.theResidualOps) {
      break;
    }
    mySum += myIt->theResidualOps;
  }

  aTask.theResidualOps -= mySum;
  if (myIt != theActive.end()) {
    assert(myIt->theResidualOps > aTask.theResidualOps);
    myIt->theResidualOps -= aTask.theResidualOps;
  }
  theActive.emplace(myIt, aTask);
}

LambdaRequirements Container::requirements(const LambdaRequest& aReq) const {
  const auto myRequirements = theLambda.requirements(theProcessor, aReq);
  if (myRequirements.theMemory > theProcessor.memTotal()) {
    throw std::runtime_error("Container " + theName +
                             " cannot handle request " + aReq.theName +
                             " since the memory requirements exceed the total "
                             "available in the processor: " +
                             std::to_string(theProcessor.memTotal()) +
                             " <= " + std::to_string(myRequirements.theMemory));
  }
  return myRequirements;
}

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&            aStream,
                         const uiiit::edge::Task& aTask) {
  aStream << "id " << aTask.theId << ", "
          << "memory " << aTask.theMemory << " bytes, "
          << "residual " << aTask.theResidualOps << " operations";
  return aStream;
}

std::ostream& operator<<(std::ostream&                 aStream,
                         const uiiit::edge::Container& aContainer) {
  aStream << "name " << aContainer.name() << ", "
          << "lambda " << aContainer.lambda() << ", "
          << "num-workers " << aContainer.numWorkers() << ", "
          << "active " << aContainer.active() << ", "
          << "pending " << aContainer.pending();
  return aStream;
}
