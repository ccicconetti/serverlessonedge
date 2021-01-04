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

#include "StateSim/job.h"

#include "Support/split.h"
#include "Support/tostring.h"

#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>

#include <glog/logging.h>

namespace uiiit {
namespace statesim {

class FunctionPicker
{
 public:
  FunctionPicker(const std::map<std::string, double>& aWeights,
                 const size_t                         aSeed)
      : theGenerator(aSeed)
      , theRv(0.0, sum(aWeights))
      , theWeights(aWeights) {
    assert(not aWeights.empty());
  }

  std::string operator()() {
    const auto myRandom = theRv(theGenerator);
    double     mySum    = 0;
    for (const auto& elem : theWeights) {
      mySum += elem.second;
      if (myRandom < mySum) {
        return elem.first;
      }
    }
    return theWeights.rbegin()->first;
  }

  static double sum(const std::map<std::string, double>& aWeights) {
    double ret = 0;
    for (const auto& elem : aWeights) {
      ret += elem.second;
    }
    return ret;
  }

 private:
  std::default_random_engine             theGenerator;
  std::uniform_real_distribution<double> theRv;
  const std::map<std::string, double>&   theWeights;
};

Job::Job(const size_t               aId,
         const double               aStart,
         const std::vector<Task>&   aTasks,
         const std::vector<size_t>& aStateSizes,
         const size_t               aRetSize)
    : theId(aId)
    , theStart(aStart)
    , theTasks(aTasks)
    , theStateSizes(aStateSizes)
    , theRetSize(aRetSize) {
  // noop
}

std::string Job::toString() const {
  std::stringstream myStream;
  myStream << "#" << theId << ", at " << theStart << " s, return " << theRetSize
           << " bytes, states {"
           << ::toString(
                  theStateSizes,
                  ",",
                  [](const auto aValue) { return std::to_string(aValue); })
           << "} bytes, tasks ["
           << ::toString(theTasks,
                         "; ",
                         [](const auto& aValue) { return aValue.toString(); })
           << "]";
  return myStream.str();
}

std::vector<Job> loadJobs(const std::string&                   aPath,
                          const double                         aOpsFactor,
                          const double                         aMemFactor,
                          const std::map<std::string, double>& aFuncWeights,
                          const size_t                         aSeed,
                          const bool                           aStatefulOnly) {
  if (aFuncWeights.empty()) {
    throw std::runtime_error("Empty function weights");
  }

  std::ifstream myFile(aPath);
  if (not myFile) {
    throw std::runtime_error("Cannot open file for reading: " + aPath);
  }

  struct TaskData {
    std::set<size_t> thePrecedences;
    size_t           theOps;
    size_t           theSize;
  };
  struct JobData {
    size_t                theId;
    double                theStartTime;
    std::vector<TaskData> theTasks;
  };
  struct Algo {
    static void findChain(const std::vector<TaskData>& aTasks,
                          std::list<size_t>&           aChain,
                          const size_t                 aCurTask) {
      for (size_t i = 0; i < aTasks.size(); i++) {
        if (aTasks[i].thePrecedences.count(aCurTask) == 1) {
          aChain.emplace_back(i);
          findChain(aTasks, aChain, i);
        }
      }
    }
  };

  // parse file, save data into temp data structures
  size_t               myNextJobId = 0;
  size_t               myLastJobId = 0;
  std::string          myLine;
  std::vector<JobData> theJobs;
  while (myFile) {
    std::getline(myFile, myLine);
    if (myLine.empty()) {
      break;
    }
    const auto myTokens = support::split<std::vector<std::string>>(myLine, ",");
    if (myTokens.size() != 7 or myTokens[1].size() < 2 or
        myTokens[1].substr(0, 2) != "j_" or myTokens[2].empty()) {
      throw std::runtime_error("Invalid task: " + myLine);
    }
    const auto myCurJobId =
        std::stoull(myTokens[1].substr(2, std::string::npos));

    // skip strange tasks
    if (myTokens[2].find("task_") != std::string::npos) {
      continue;
    }

    if (myLastJobId != myCurJobId) {
      theJobs.emplace_back(JobData{myNextJobId, std::stod(myTokens[0]), {}});
      myNextJobId++;
    }
    myLastJobId = myCurJobId;

    const auto                  myDuration  = std::stoul(myTokens[3]);
    const auto                  myCpu       = std::stod(myTokens[4]);
    const auto                  myMem       = std::stod(myTokens[5]);
    [[maybe_unused]] const auto myInstances = std::stoul(myTokens[6]);

    const auto myPrecTokens = support::split<std::vector<std::string>>(
        myTokens[2].substr(1, std::string::npos), "_");
    if (myPrecTokens.empty()) {
      throw std::runtime_error("Invalid task: " + myLine);
    }
    auto myTaskId = std::stoull(myPrecTokens[0]);
    if (myTaskId == 0) {
      throw std::runtime_error("Invalid task identifier (0): " + myLine);
    }
    myTaskId--;
    auto& myJob = theJobs.back();
    if (myJob.theTasks.size() <= myTaskId) {
      myJob.theTasks.resize(myTaskId + 1);
    }
    auto& myTask = myJob.theTasks[myTaskId];
    for (size_t i = 1; i < myPrecTokens.size(); i++) {
      myTask.thePrecedences.insert(std::stoull(myPrecTokens[i]) - 1);
    }
    myTask.theOps = static_cast<size_t>(0.5 + 1.0 * myDuration * myCpu / 100.0 *
                                                  aOpsFactor);
    myTask.theSize = static_cast<size_t>(0.5 + myMem * aMemFactor);
  }

  // add all the jobs
  std::vector<Job> myJobs;
  FunctionPicker   myFunctionPicker(aFuncWeights, aSeed);
  for (const auto& myJob : theJobs) {
    VLOG(1) << "job " << myJob.theId << " at " << myJob.theStartTime;
    size_t i = 0;
    for (const auto& myTask : myJob.theTasks) {
      VLOG(1) << "  "
              << "task " << i++ << ", ops " << myTask.theOps << ", size "
              << myTask.theSize << ", prec "
              << ::toString(myTask.thePrecedences, " ", [](const auto aValue) {
                   return std::to_string(aValue);
                 });
    }

    assert(not myJob.theTasks.empty());

    // find the processing chain starting from task 0
    std::list<size_t> myChainWithoutZero;
    Algo::findChain(myJob.theTasks, myChainWithoutZero, 0);
    std::vector<size_t> myChain;
    std::set<size_t>    myTasksInChain;
    myChain.emplace_back(0);
    myTasksInChain.insert(0);
    for (const auto myId : myChainWithoutZero) {
      myChain.emplace_back(myId);
      myTasksInChain.insert(myId);
    }

    // find state dependencies of the tasks along the chain
    std::map<size_t, std::set<size_t>> myStateDependencies;
    std::set<size_t>                   myAllStates;
    auto                               myStatelessJob = true;
    for (const auto myId : myChain) {
      myStateDependencies[myId] = std::set<size_t>();
      for (const auto myPrec : myJob.theTasks[myId].thePrecedences) {
        if (myTasksInChain.count(myPrec) == 1) {
          // skip tasks in the chain
        } else {
          // all tasks not in the chain are state dependencies
          myStatelessJob = false;
          myStateDependencies[myId].insert(myPrec);
          myAllStates.insert(myPrec);
        }
      }
    }

    // discard job only jobs with at least one stateful task are required
    // and there are none
    if (aStatefulOnly and myStatelessJob) {
      VLOG(1) << "discarding job: all tasks are stateless";
      continue;
    }

    VLOG(1) << "  chain: " << ::toString(myChain, " ", [](const auto aValue) {
      return std::to_string(aValue);
    });
    VLOG(1) << "  state deps: "
            << ::toString(myStateDependencies, " ", [](const auto& aPair) {
                 return std::to_string(aPair.first) + " [" +
                        ::toString(aPair.second,
                                   "|",
                                   [](const auto aValue) {
                                     return std::to_string(aValue);
                                   }) +
                        "]";
               });

    // find new identifiers starting from 0 for both the tasks and the states
    struct NewId {
      std::map<size_t, size_t>
      operator()(const std::set<size_t>& aElements) const {
        std::map<size_t, size_t> ret;
        size_t                   myCounter = 0;
        for (const auto myId : aElements) {
          ret[myId] = myCounter++;
        }
        return ret;
      }
    };
    auto                        myTaskIdMap      = NewId()(myTasksInChain);
    auto                        myStateIdMap     = NewId()(myAllStates);
    [[maybe_unused]] const auto myTaskIdMapSize  = myTaskIdMap.size();
    [[maybe_unused]] const auto myStateIdMapSize = myStateIdMap.size();

    // initialize the vector of tasks
    std::vector<Task> myTasks;
    for (const auto myTaskId : myTasksInChain) {
      std::vector<size_t> myDeps;
      assert(myStateDependencies.find(myTaskId) != myStateDependencies.end());
      for (const auto myStateId : myStateDependencies[myTaskId]) {
        myDeps.emplace_back(myStateIdMap[myStateId]);
      }
      myTasks.emplace_back(Task(myTaskIdMap[myTaskId],
                                myJob.theTasks[myTaskId].theSize,
                                myJob.theTasks[myTaskId].theOps,
                                myFunctionPicker(),
                                myDeps));
    }

    // initialize the size of the states
    std::vector<size_t> myStateSizes(myStateIdMap.size());
    for (const auto myStateId : myAllStates) {
      myStateSizes[myStateIdMap[myStateId]] = myJob.theTasks[myStateId].theSize;
    }

    // check that the id mapping maps have not been increased by mistake
    assert(myTaskIdMapSize == myTaskIdMap.size());
    assert(myStateIdMapSize == myStateIdMap.size());

    // add the new job
    myJobs.emplace_back(Job{myJobs.size(),
                            myJob.theStartTime,
                            myTasks,
                            myStateSizes,
                            myTasks[0].size()});
  }

  return myJobs;
}

} // namespace statesim
} // namespace uiiit
