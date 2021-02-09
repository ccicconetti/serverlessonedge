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

#include "Support/chrono.h"
#include "Support/movingavg.h"
#include "processortype.h"

#include <array>
#include <iostream>
#include <string>

namespace uiiit {
namespace edge {

/**
 * Abstraction of a processor with associated memory.
 * Can be a multi-core CPU system or an acceleration board such as a GPU.
 */
class Processor final
{
 public:
  /**
   * \param aName The processor name.
   * \param aType The processor type.
   * \param aSpeed The speed, in number of operations per second, per core.
   * \param aCores The number of cores.
   * \param aMem The memory availability, in bytes.
   *
   * \throw std::runtime_error if the name is empty or the speed is not positive
   *        or the number of cores is zero or the memory size is zero
   */
  explicit Processor(const std::string&  aName,
                     const ProcessorType aType,
                     const float         aSpeed,
                     const size_t        aCores,
                     const uint64_t      aMem);

  /**
   * Allocate some memory.
   *
   * \throw std::runtime_error if the amount of memory requested is not
   * currently available.
   */
  void allocate(const uint64_t aSize);

  /**
   * Free some memory.
   *
   * \throw if the amount of memory freed was not allocated.
   */
  void free(const uint64_t aSize);

  /**
   * \return the time required to perform the given number of operations,
   * provided that the set of active tasks remains the same as it is now and
   * each task gets 1/N of the overall processor speed, where N is the minimum
   * between the number of cores and the number of tasks. The rationale is that
   * if there are few tasks (< number of cores) then they will not be able to
   * use more than one core each; on the other hand, if there are at least as
   * many tasks as cores then the overall speed is equally shared between the
   * active tasks. If there are no running tasks then return 0.
   *
   * \param aOps the number of operations.
   */
  double opsToTime(const uint64_t aOps) const noexcept;

  /**
   * Same as optsToTime() but pretending that there was one more task running.
   */
  double opsToTimePlusOne(const uint64_t aOps) const noexcept;

  /**
   * \return the number of operations that can be done in the given time, under
   * the same assumptions as opsToTime(). If there are no running tasks then
   * return 0.
   *
   * \param aTime the time, in seconds.
   */
  uint64_t timeToOps(const double aTime) const noexcept;

  //! \return the real time utilization since the last call to this method.
  double utilization();

  //! \return the average loads in the last 1, 10, and 30 seconds.
  std::array<double, 3> lastUtils() const noexcept;

  //! \return true if there are no tasks running on the processor.
  bool idle() const noexcept {
    return theRunning == 0;
  }

  //! \return the number of tasks running on the processor.
  size_t running() const noexcept {
    return theRunning;
  }

  //! \return the processor name
  const std::string& name() const noexcept {
    return theName;
  }

  //! \return the processor type
  ProcessorType type() const noexcept {
    return theType;
  }

  //! \return the processor speed, in number of operations per second, per core
  float speed() const noexcept {
    return theSpeed;
  }

  //! \return the number of cores
  size_t cores() const noexcept {
    return theCores;
  }

  //! \return the total main memory, in bytes
  uint64_t memTotal() const noexcept {
    return theMemTotal;
  }

  //! \return the amount of memory currently available, in bytes
  uint64_t memAvailable() const noexcept {
    return theMemTotal - theMemUsed;
  }

  //! \return the amount of memory currently used, in bytes
  uint64_t memUsed() const noexcept {
    return theMemUsed;
  }

 private:
  /**
   * \return the speed that can be currently allocated to one task, in ops/s.
   *
   * \pre theRunning > 0
   */
  double equivalentSpeed() const noexcept;

 private:
  // constant configuration from ctor
  const std::string   theName;
  const ProcessorType theType;
  const float         theSpeed;
  const size_t        theCores;
  const uint64_t      theMemTotal;

  // amount of memory currently in use, in bytes
  // theMemUser == 0 iff theRunning == 0
  uint64_t theMemUsed;

  // number of tasks running
  size_t theRunning;

  // measures the time between consecutive calls of the method utilization()
  support::Chrono theChrono;

  // measures the time between consecutive calls to allocate()/free()
  support::Chrono theBusyChrono;

  // sum of busy times weighted on the number of running jobs
  double theBusyTime;

  // moving average windows every 10 and 30 seconds, resp.
  support::MovingAvg<double> theLoad10Wnd;
  support::MovingAvg<double> theLoad30Wnd;
};

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                 aStream,
                         const uiiit::edge::Processor& aProcessor);
