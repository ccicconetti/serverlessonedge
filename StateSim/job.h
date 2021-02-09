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

#include "StateSim/task.h"

#include <map>
#include <string>
#include <vector>

namespace uiiit {
namespace statesim {

class Job
{
 public:
  /**
   *
   * Create a job.
   *
   * \param aId The job identifier, unique within a scenario.
   *
   * \param aStart The job start time, relative, in seconds.
   *
   * \param aTasks The tasks this job is made of.
   *
   * \param aStateSizes The size of the states required, in bytes.
   *
   * \param aRetSize The size of the return value, in bytes.
   *
   * \throw std::runtime_error if there are no tasks or an inconsistency is
   *        found in tasks, such as there is a dependency from a non-existing
   *        state
   */
  explicit Job(const size_t               aId,
               const double               aStart,
               const std::vector<Task>&   aTasks,
               const std::vector<size_t>& aStateSizes,
               const size_t               aRetSize);

  //! \return a job identical but with different identifier.
  static Job clone(const Job& aAnother, const size_t aId);

  //! \return a human-readable string.
  std::string toString() const;

  // clang-format off
  size_t id()                             const noexcept { return theId;         }
  double start()                          const noexcept { return theStart;      }
  const std::vector<Task>& tasks()        const noexcept { return theTasks;      }
  const std::vector<size_t>& stateSizes() const noexcept { return theStateSizes; }
  size_t retSize()                        const noexcept { return theRetSize;    }
  // clang-format on

 private:
  const size_t              theId;
  const double              theStart;
  const std::vector<Task>   theTasks;
  const std::vector<size_t> theStateSizes;
  const size_t              theRetSize;
};

/**
 * Load the jobs from a given file.
 *
 * \param aPath The file from which to load the jobs. Must be in the format
 *        used by Alibaba traces (https://github.com/All-less/trace-generator)
 *        in the batch_task.csv file
 *
 * \param aOpsFactor The multiplicative factor to determine the number
 *        of operations of a task
 *
 * \param aMemFactor The multiplicative factor to determine the size of the
 *        states and the intermediate function input/output sizes
 *
 * \param aFuncWeights The function names to pick randomly, with weights
 *
 * \param aSeed The seed to initialize the random number generator
 *
 * \param aStatefulOnly Discard jobs that have only stateless tasks
 *
 * \return A vector with all the jobs loaded. Can be empty
 *
 * \throw std::runtime_error if aPath cannot be opened or aFuncWeights is empty
 */
std::vector<Job> loadJobs(const std::string&                   aPath,
                          const double                         aOpsFactor,
                          const double                         aMemFactor,
                          const std::map<std::string, double>& aFuncWeights,
                          const size_t                         aSeed,
                          const bool                           aStatefulOnly);

} // namespace statesim
} // namespace uiiit
