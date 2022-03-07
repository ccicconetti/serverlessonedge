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

#pragma once

#include "LambdaMuSim/scenario.h"
#include "Support/macros.h"
#include "Support/queue.h"
#include "Support/threadpool.h"

#include <boost/filesystem.hpp>

#include <deque>
#include <mutex>
#include <vector>

namespace uiiit {
namespace lambdamusim {

struct Conf {
  enum class Type : uint16_t {
    Snapshot = 0, //!< single snapshot
    Dynamic  = 1, //!< dynamic flow simulation
  };

  //! Simulation type.
  Type theType;

  //! File containing the info about nodes
  const std::string theNodesPath;
  //! File containing the info about links
  const std::string theLinksPath;
  //! File containing the info about edges
  const std::string theEdgesPath;
  //! Cloud distance scale fctor
  const double theCloudDistanceFactor;
  //! File containing the info about apps (unused with Type::Snapshot)
  const std::string theAppsPath;
  //! Simulation duration (unused with Type::Snapshot)
  const double theDuration;
  //! Warm-up duration (unused with Type::Snapshot)
  const double theWarmUp;
  //! Epoch duration (unused with Type::Snapshot)
  const double theEpoch;
  //! Only use apps from dataset with at least these periods (unused with
  //! Type::Snapshot)
  const std::size_t theMinPeriods;
  //! Average number of apps (unused with Type::Snapshot)
  const std::size_t theAvgApps;
  //! Average number of lambda apps (unused with Type::Dynamic)
  const std::size_t theAvgLambda;
  //! Average number of mu apps (unused with Type::Dynamic)
  const std::size_t theAvgMu;
  //! Factor to reserve containers to lambda-apps, in [0,1]
  const double theAlpha;
  //! Factor to overprovision the capacity for lambda-apps, in [0,1]
  const double theBeta;
  //! Capacity requested by each lambda.
  const long theLambdaRequest;

  //! File where to save performance data (can be empty)
  const std::string theOutfile;
  //! Whether the output file should be appended or replaced.
  const bool theAppend;

  std::vector<std::string>               toStrings() const;
  static const std::vector<std::string>& toColumns();

  std::string type() const;
};

struct Desc {
  // RNG seed
  std::size_t theSeed;

  // configuration
  const Conf* theConf;

  // apps' periods
  const std::vector<std::deque<double>>* theAppPeriods;

  // simulation scenario
  std::unique_ptr<Scenario> theScenario;

  std::string toString() const;
};

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

  //! Simulation output data.
  using Data = std::deque<std::tuple<std::size_t, PerformanceData>>;

 public:
  //! Create a simulation environment.
  explicit Simulation(const size_t aNumThreads);

  ~Simulation();

  /**
   *  Run a batch of simulations.
   */
  void run(const Conf&  aConf,
           const size_t aStartingSeed,
           const size_t aNumReplications);

 private:
  //! Save current performance data to the given file.
  static void save(const Conf& aConf, const Data& aData);

 private:
  mutable std::mutex          theMutex;
  const size_t                theNumThreads;
  support::ThreadPool<Worker> theWorkers;
  support::Queue<size_t>      theQueueIn;
  support::Queue<bool>        theQueueOut;
  std::vector<Desc>           theDesc;
  Data                        theData;
};

} // namespace lambdamusim
} // namespace uiiit
