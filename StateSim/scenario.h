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

#pragma once

#include "StateSim/affinity.h"
#include "StateSim/job.h"
#include "StateSim/network.h"
#include "Support/macros.h"

#include <fstream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>

namespace uiiit {
namespace statesim {

struct PerformanceData {
  struct Job {
    //! Processing time, in s.
    double theProcDelay;
    //! Network delay, in s.
    double theNetDelay;
    //! Amount of data transferred, in bytes.
    size_t theDataTransfer;
    //! Number of tasks in the chain.
    size_t theChainSize;

    explicit Job();
    explicit Job(const double aProcDelay,
                 const double aNetDelay,
                 const size_t aDataTransfer,
                 const size_t aChainSize);

    //! Sum processing/network delays and the data transferred.
    void merge(const Job& aOther) noexcept;

    //! \return a string with the space-separated values.
    std::string toString() const;

    //! \return true if the two structs are bit-wise identical.
    bool operator==(const Job& aOther) const noexcept;
  };
  //! Data for each job.
  std::vector<Job> theJobData;
  //! For each processing node, the number of tasks assigned.
  std::vector<size_t> theLoad;

  //! \return the number of jobs.
  size_t numJobs() const noexcept {
    return theJobData.size();
  }

  //! \return the number of (processing) nodes.
  size_t numNodes() const noexcept {
    return theLoad.size();
  }

  bool operator==(const PerformanceData& aOther) const;

  void                   save(std::ofstream& aOutput) const;
  static PerformanceData load(std::ifstream& aInput);

  static constexpr size_t theVersion = 2;
};

enum class Policy : int {
  PureFaaS       = 0,
  StatePropagate = 1,
  StateLocal     = 2,
};

class Scenario
{
  NONCOPYABLE_NONMOVABLE(Scenario);

  using Allocation = std::vector<std::vector<Node*>>;

 public:
  struct Conf {
    //! File containing the info about nodes
    const std::string& theNodesPath;
    //! File containing the info about links
    const std::string theLinksPath;
    //! File containing the info about edges
    const std::string theEdgesPath;
    //! File containing the info about tasks
    const std::string theTasksPath;
    //! Multiplier for number of operations of tasks
    const double theOpsFactor;
    //! Multiplier for argument sizes
    const double theArgFactor;
    //! Multiplier for state sizes
    const double theStateFactor;
    //! Weights to draw randomly assignment of lambda functions
    const std::map<std::string, double>& theFuncWeights;
    //! Seed to initialize random number generation
    const size_t theSeed;
    //! True if only stateful jobs are considered
    const bool theStatefulOnly;
    //! Weights to draw randomly affinity of lambda functions
    const std::map<Affinity, double> theAffinityWeights;
  };

  //! Create a scenario with a given configuration.
  explicit Scenario(const Conf& aConf);

  //! Create a scenario with the given structures.
  explicit Scenario(const std::map<std::string, Affinity>& aAffinities,
                    const std::shared_ptr<Network>&        aNetwork,
                    const std::vector<Job>&                aJobs,
                    const size_t                           aSeed);

  //! \return the seed of this scenario
  size_t seed() const noexcept {
    return theSeed;
  }

  /**
   * Allocate the tasks of all jobs to processing nodes in the network using
   * a shortest processing time first.
   *
   * \param aPolicy The policy used
   */
  void allocateTasks(const Policy aPolicy);

  //! \return the execution and transmission delays of jobs with a given policy
  PerformanceData performance(const Policy aPolicy) const;

 private:
  /**
   * Return the execution time (processing vs. network) if a new task
   * is allocated to the candidate node.
   *
   * \param aOps The number of operations of the task
   *
   * \param aInSize The total input size (argument + state, if any)
   *
   * \param aOutSize The total output size (argument + state, if any)
   *
   * \param aClient The client node
   *
   * \param aNode The candidate node
   *
   * \return execution time (network transfer, processing), in s
   */
  std::pair<double, double> execTimeNew(const size_t aOps,
                                        const size_t aInSize,
                                        const size_t aOutSize,
                                        const Node&  aClient,
                                        const Node&  aNode);

  /**
   * Return the execution time (processing vs. network) of a given task
   * allocated to a candidate node when transferring the given amount
   * of data from the origin (in) and then back to the origin (out), and
   * the total amount of the data transferred in the network.
   *
   * \param aOps The number of operations of the task
   *
   * \param aInSize The total input size (argument + state, if any)
   *
   * \param aOutSize The total output size (argument + state, if any)
   *
   * \param aOrigin The origin node
   *
   * \param aTarget The node to which this task is allocated
   *
   * \return execution time (network transfer, processing), in s, and data
   * transferred, in bytes
   */
  PerformanceData::Job execStatsTwoWay(const size_t aOps,
                                       const size_t aInSize,
                                       const size_t aOutSize,
                                       const Node&  aOrigin,
                                       const Node&  aTarget) const;
  /**
   * Return the execution time (processing vs. network) of a given task
   * allocated to a candidate node when transferring the given amount
   * of data from the origin (in), and
   * the total amount of the data transferred in the network.
   *
   * \param aOps The number of operations of the task (can be zero)
   *
   * \param aSize The transfer size (if any)
   *
   * \param aOrigin The origin node
   *
   * \param aTarget The node to which this task is allocated
   *
   * \return execution time (network transfer, processing), in s, and data
   * transferred, in bytes
   */
  PerformanceData::Job execStatsOneWay(const size_t aOps,
                                       const size_t aSize,
                                       const Node&  aOrigin,
                                       const Node&  aTarget) const;

  //! \return a random order of the job identifiers
  std::vector<size_t> shuffleJobIds();

  //! \return a random client assignment
  static std::vector<Node*> randomClients(const size_t                aNumJobs,
                                          const Network&              aNetwork,
                                          std::default_random_engine& aRng);

  //! \return the sum of all the job states and the input vs. output of a task.
  static std::pair<size_t, size_t> allStatesArgSizes(const Job&  aJob,
                                                     const Task& aTask);

 private:
  std::default_random_engine            theRng;
  const size_t                          theSeed;
  const std::map<std::string, Affinity> theAffinities;
  const std::shared_ptr<Network>        theNetwork;
  std::vector<Job>                      theJobs;

  // for each job this is the client node picked at random
  std::vector<Node*> theClients;

  // one load per node (only > 0 for processing nodes)
  std::vector<size_t> theLoad;

  // for each job, for each task, this is the node allocated
  Allocation theAllocation;
};

std::string             toString(const Policy aPolicy);
Policy                  policyFromString(const std::string& aValue);
const std::set<Policy>& allPolicies();

} // namespace statesim
} // namespace uiiit
