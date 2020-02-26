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

#include "lambda.h"

#include <iostream>
#include <list>
#include <memory>

namespace uiiit {
namespace edge {

class Processor;
struct LambdaRequest;
struct LambdaResponse;

//! A task performed by a worker in a contaier.
struct Task {
  explicit Task(const uint64_t aId,
                const uint64_t aMemory,
                const uint64_t aResidualOps);

  const uint64_t theId;
  const uint64_t theMemory;

  uint64_t                              theResidualOps;
  std::shared_ptr<const LambdaResponse> theResp;
};

/**
 * Abstraction of a light-weight virtual machine that hosts a number of workers,
 * all executing the same lambda function.
 */
class Container final
{
 public:
  /**
   * \param aName The container name.
   * \param aProcessor The processor where this container is hosted.
   * \param aLambda The function that this container executes.
   * \param aNumWorkers The number of available workers. This value limits the
   * concurrency level within the container.
   *
   * \throw std::runtime_error if the number of workers is zero.
   */
  explicit Container(const std::string& aName,
                     Processor&         aProcessor,
                     const Lambda&      aLambda,
                     const size_t       aNumWorkers);

  /**
   * Add a new task to this container.
   * If there are available workers then the request is activated immediately,
   * otherwise execution is postponed until a worker is available. Requests are
   * handled on a first come first serve basis.

   * \param aReq The lambda request, containinig the input.
   * \param aId A unique identifier of the request, used to identify task
   * completion.
   *
   * \throw std::runtime_error if the task would require more memory than the
   * total available in the processor.
   */
  void push(const LambdaRequest& aReq, uint64_t aId);

  /**
   * Simulate the addition of the given task to this container.
   *
   * The memory requirements are ignored (it would be very complex to handle
   * them since memory on the processor is reserved also by other containers).
   *
   * \return the simulated time required for completion.
   */
  double simulate(const LambdaRequest& aReq) const;

  //! \return the average loads in the last 1, 10, and 30 seconds.
  std::array<double, 3> lastUtils() const noexcept;

  /**
   * Return the task whose execution time is earliest.
   * This automatically advances the computation of all the active tasks and
   * may activate one or more pending tasks.
   *
   * \throw std::runtime_error if there are no active tasks.
   */
  Task pop();

  /**
   * Performe a number of operations without removing any active task.
   *
   * The number of operations depends on the specified time elapsed.
   *
   * The method has no effect if there are no active tasks.
   *
   * \param aElapsed the time elapsed, in seconds.
   *
   * \throw std::runtime_error if aElapsed is negative
   */
  void advance(const double aElapsed);

  //! \return the number of active tasks.
  size_t active() const noexcept {
    return theActive.size();
  }

  //! \return the number of pending tasks.
  size_t pending() const noexcept {
    return thePending.size();
  }

  /**
   * \return the residual processing time of the task nearest to completion.
   *
   * \throw std::runtime_error if there are no active tasks.
   */
  double nearest() const;

  // accessors
  const std::string& name() const noexcept {
    return theName;
  }
  Processor& processor() {
    return theProcessor;
  }
  const Lambda& lambda() const noexcept {
    return theLambda;
  }
  size_t numWorkers() const noexcept {
    return theNumWorkers;
  }

 private:
  void               throwIfEmpty() const;
  void               makeActive(Task&& aTask);
  LambdaRequirements requirements(const LambdaRequest& aReq) const;

 private:
  const std::string theName;
  Processor&        theProcessor;
  const Lambda      theLambda;
  const size_t      theNumWorkers;

  std::list<Task> theActive;
  std::list<Task> thePending;
};

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream& aStream, const uiiit::edge::Task& aTask);
std::ostream& operator<<(std::ostream&                 aStream,
                         const uiiit::edge::Container& aContainer);
