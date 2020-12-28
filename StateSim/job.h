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

#include "StateSim/task.h"

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
   * \param aId The task identifier, unique within a job.
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

std::vector<Job> loadJobs(const std::string& aPath,
                          const double       aOpsFactor,
                          const double       aMemFactor,
                          const size_t       aSeed);

} // namespace statesim
} // namespace uiiit
