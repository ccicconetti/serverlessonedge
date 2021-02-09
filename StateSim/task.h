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

#include <string>
#include <vector>

namespace uiiit {
namespace statesim {

class Task
{
 public:
  /**
   *
   * Create a task.
   *
   * \param aId The task identifier, unique within a job.
   *
   * \param aSize The size of the task argument, in bytes.
   *
   * \param aOps The number of operations required for this task.
   *
   * \param aFunction The name of the function to execute.
   *
   * \param aDeps The set of states on which this task depends. Can be empty.
   */
  explicit Task(const size_t              aId,
                const size_t              aSize,
                const size_t              aOps,
                const std::string&        aFunc,
                const std::vector<size_t> aDeps);

  //! \return a human-readable string.
  std::string toString() const;

  // clang-format off
  size_t id()                       const noexcept { return theId;   }
  size_t size()                     const noexcept { return theSize; }
  size_t ops()                      const noexcept { return theOps;  }
  const std::string& func()         const noexcept { return theFunc; }
  const std::vector<size_t>& deps() const noexcept { return theDeps; }
  // clang-format on

 private:
  const size_t              theId;
  const size_t              theSize;
  const size_t              theOps;
  const std::string         theFunc;
  const std::vector<size_t> theDeps;
};

} // namespace statesim
} // namespace uiiit
