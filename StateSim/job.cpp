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

#include "StateSim/job.h"

#include "Support/split.h"
#include "Support/tostring.h"

#include <glog/logging.h>

#include <fstream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>

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

Job Job::clone(const Job& aAnother, const size_t aId) {
  return Job(aId,
             aAnother.theStart,
             aAnother.theTasks,
             aAnother.theStateSizes,
             aAnother.theRetSize);
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

struct Row {
  double      theStartTime;
  std::string theJobName;
  std::string theTaskName;
  uint32_t    theDuration;
  double      theCpu;
  double      theMem;
  uint32_t    theNumInstances;

  bool theSkip = false;
};

Row parseRow(const std::string& aLine, const BatchTaskFormat aFormat) {
  Row ret;
  if (aFormat == BatchTaskFormat::Spar) {
    const auto myTokens = support::split<std::vector<std::string>>(aLine, ",");
    if (myTokens.size() != 7 or myTokens[1].size() < 2 or
        myTokens[1].substr(0, 2) != "j_" or myTokens[2].empty()) {
      VLOG(2) << "skipping invalid task: " << aLine;
      ret.theSkip = true;
    } else {
      ret.theStartTime    = std::stod(myTokens[0]);
      ret.theJobName      = myTokens[1];
      ret.theTaskName     = myTokens[2];
      ret.theDuration     = std::stoul(myTokens[3]);
      ret.theCpu          = std::stod(myTokens[4]);
      ret.theMem          = std::stod(myTokens[5]);
      ret.theNumInstances = std::stoul(myTokens[6]);
    }

  } else if (aFormat == BatchTaskFormat::Alibaba) {
    const auto myTokens = support::split<std::vector<std::string>>(aLine, ",");
    if (myTokens.size() != 9 or myTokens[2].size() < 2 or
        myTokens[2].substr(0, 2) != "j_" or myTokens[0].empty()) {
      VLOG(2) << "skipping invalid task: " << aLine;
      ret.theSkip = true;
    } else {
      ret.theStartTime    = std::stod(myTokens[5]);
      ret.theJobName      = myTokens[2];
      ret.theTaskName     = myTokens[0];
      ret.theDuration     = ret.theStartTime - std::stod(myTokens[6]);
      ret.theCpu          = std::stod(myTokens[7]);
      ret.theMem          = std::stod(myTokens[8]);
      ret.theNumInstances = std::stoul(myTokens[1]);
    }

  } else {
    throw std::runtime_error("Unknown batch_task.csv trace format");
  }

  static const std::set<std::string> myStrangeKeywords(
      {"task_", "MergeTask", "Stg"});
  for (const auto& myKeyword : myStrangeKeywords) {
    if (ret.theTaskName.find(myKeyword) != std::string::npos) {
      VLOG(2) << "skipping strange task: " << aLine;
      ret.theSkip = true;
      break;
    }
  }

  return ret;
}

std::vector<Job> loadJobs(const std::string&                   aPath,
                          const double                         aOpsFactor,
                          const double                         aArgFactor,
                          const double                         aStateFactor,
                          const std::map<std::string, double>& aFuncWeights,
                          const size_t                         aSeed,
                          const bool                           aStatefulOnly,
                          const BatchTaskFormat aBatchTaskFormat) {
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
    double           theSize;
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
  std::vector<JobData> myJobsData;
  while (myFile) {
    std::getline(myFile, myLine);
    if (myLine.empty()) {
      break;
    }
    const auto myRow = parseRow(myLine, aBatchTaskFormat);

    // skip strange tasks
    if (myRow.theSkip) {
      continue;
    }

    assert(myRow.theJobName.size() > 2);
    const auto myCurJobId =
        std::stoull(myRow.theJobName.substr(2, std::string::npos));

    if (myLastJobId != myCurJobId) {
      myJobsData.emplace_back(JobData{myNextJobId, myRow.theStartTime, {}});
      myNextJobId++;
    }
    myLastJobId = myCurJobId;

    const auto myPrecTokens = support::split<std::vector<std::string>>(
        myRow.theTaskName.substr(1, std::string::npos), "_");
    if (myPrecTokens.empty()) {
      throw std::runtime_error("Invalid task: " + myLine);
    }
    auto myTaskId = std::stoull(myPrecTokens[0]);
    if (myTaskId == 0) {
      throw std::runtime_error("Invalid task identifier (0): " + myLine);
    }
    myTaskId--;
    auto& myJob = myJobsData.back();
    if (myJob.theTasks.size() <= myTaskId) {
      myJob.theTasks.resize(myTaskId + 1);
    }
    auto& myTask = myJob.theTasks[myTaskId];
    for (size_t i = 1; i < myPrecTokens.size(); i++) {
      myTask.thePrecedences.insert(std::stoull(myPrecTokens[i]) - 1);
    }
    myTask.theOps = static_cast<size_t>(
        0.5 + 1.0 * myRow.theDuration * myRow.theCpu / 100.0 * aOpsFactor);
    myTask.theSize = myRow.theMem; // scale factor to be applied later
  }

  // add all the jobs
  std::vector<Job> myJobs;
  FunctionPicker   myFunctionPicker(aFuncWeights, aSeed);
  for (const auto& myJob : myJobsData) {
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
      myTasks.emplace_back(
          Task(myTaskIdMap[myTaskId],
               static_cast<size_t>(0.5 + aArgFactor *
                                             myJob.theTasks[myTaskId].theSize),
               myJob.theTasks[myTaskId].theOps,
               myFunctionPicker(),
               myDeps));
    }

    // initialize the size of the states
    std::vector<size_t> myStateSizes(myStateIdMap.size());
    for (const auto myStateId : myAllStates) {
      myStateSizes[myStateIdMap[myStateId]] = static_cast<size_t>(
          0.5 + aStateFactor * myJob.theTasks[myStateId].theSize);
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
