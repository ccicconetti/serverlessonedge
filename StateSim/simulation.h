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

#include "StateSim/scenario.h"
#include "Support/macros.h"
#include "Support/queue.h"
#include "Support/threadpool.h"

#include <boost/filesystem.hpp>

namespace uiiit {
namespace statesim {

class Simulation final
{
  NONCOPYABLE_NONMOVABLE(Simulation);

  class Worker final
  {
   public:
    Worker(Simulation& aSimulation);
    void operator()();
    void stop();

   private:
    Simulation& theSimulation;
  };

  struct Desc {
    // conf
    std::unique_ptr<Scenario> theScenario;
    AllocPolicy               theAllocPolicy;
    ExecPolicy                theExecPolicy;

    // output
    PerformanceData thePerformanceData;

    std::string toString() const;
  };

 public:
  struct Conf {
    //! File containing the info about nodes
    const std::string theNodesPath;
    //! File containing the info about links
    const std::string theLinksPath;
    //! File containing the info about edges
    const std::string theEdgesPath;
    //! File containing the info about tasks
    const std::string theTasksPath;
    //! File where to save performance data (can be empty)
    const std::string theOutfile;
    //! Directory where to save performance data (can be empty)
    const std::string theOutdir;
    //! Number of lambda functions
    const size_t theNumFunctions;
    //! Number of jobs per replication
    const size_t theNumJobs;
    //! Latency to access the cloud, in s.
    const double theCloudLatency;
    //! Transmission rate towards the cloud, in Mb/s.
    const double theCloudRate;
    //! The multiplier factor for the number of operations (tasks).
    const double theOpsFactor;
    //! The multiplier factor for the argument sizes (tasks).
    const double theArgFactor;
    //! The multiplier factor for the state sizes (tasks).
    const double theStateFactor;

    std::string toString() const;
  };

  //! Create a simulation environment.
  explicit Simulation(const size_t aNumThreads);

  ~Simulation();

  /**
   *  Run a batch of simulations, full factorial on the allocation and
   *  execution policies passed.
   */
  void run(const Conf&                  aConf,
           const size_t                 aStartingSeed,
           const size_t                 aNumReplications,
           const std::set<AllocPolicy>& aAllocPolicies,
           const std::set<ExecPolicy>&  aExecPolicies);

 private:
  /**
   * Select jobs from a pool.
   *
   * \param aJobs The jobs' pool
   *
   * \param aNumJobs The target number of jobs to be selected
   *
   * \param aRng The RNG
   */
  static std::vector<Job> selectJobs(const std::vector<Job>&     aJobs,
                                     const size_t                aNumJobs,
                                     std::default_random_engine& aRng);

  //! Save current performance data to the given file.
  void save(const std::string& aOutfile);

  //! Save current performance data to the given file.
  void saveDir(const boost::filesystem::path& aDir);

 private:
  const size_t                theNumThreads;
  support::ThreadPool<Worker> theWorkers;
  support::Queue<size_t>      theQueueIn;
  support::Queue<bool>        theQueueOut;
  std::vector<Desc>           theDesc;
};

} // namespace statesim
} // namespace uiiit
