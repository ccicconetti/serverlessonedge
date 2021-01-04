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

#pragma once

#include "StateSim/affinity.h"
#include "StateSim/job.h"
#include "StateSim/network.h"
#include "Support/macros.h"

#include <map>
#include <memory>
#include <random>
#include <string>

namespace uiiit {
namespace statesim {

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
    //! Multiplier for amount of memory/argument size of tasks
    const double theMemFactor;
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
                    std::unique_ptr<Network>&&             aNetwork,
                    const std::vector<Job>&                aJobs,
                    const size_t                           aSeed);

  /**
   * Allocate the tasks of all jobs to processing nodes in the network using
   * a shortest processing time first.
   */
  void allocateTasks();

  //! \return the execution and transmission delays of jobs.
  void performance(std::vector<double>& aProcDelays,
                   std::vector<double>& aNetDelays);

 private:
  static std::map<std::string, Affinity>
  makeAffinities(const std::map<std::string, double> aFuncWeights,
                 const std::map<Affinity, double>&   aAffinityWeights,
                 std::default_random_engine&         aRng);

  /**
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
   * \param aCandidate True if we are evaluating aNode as a possible candidate
   *        node for the execution, false otherwise
   *
   * \return execution time <network transfer,processing> if
   *         the given task, identified by the number of
   *         operations and input/output sizes (including state transfer), is
   *         executed on the given processing node
   */
  std::pair<double, double> execTime(const size_t aOps,
                                     const size_t aInSize,
                                     const size_t aOutSize,
                                     const Node&  aClient,
                                     const Node&  aNode,
                                     const bool   aCandidate);

  //! \return a random order of the job identifiers
  std::vector<size_t> shuffleJobIds();

  //! \return a random client assignment
  static std::vector<Node*> randomClients(const size_t                aNumJobs,
                                          const Network&              aNetwork,
                                          std::default_random_engine& aRng);

  //! \return the input/output transfer sizes of a task.
  static std::pair<size_t, size_t> sizes(const Job& aJob, const Task& aTask);

 private:
  std::default_random_engine            theRng;
  const std::map<std::string, Affinity> theAffinities;
  const std::unique_ptr<Network>        theNetwork;
  std::vector<Job>                      theJobs;

  // for each job this is the client node picked at random
  std::vector<Node*> theClients;

  // one load per node (only > 0 for processing nodes)
  std::vector<size_t> theLoad;

  // for each job, for each task, this is the node allocated
  Allocation theAllocation;
};

} // namespace statesim
} // namespace uiiit
