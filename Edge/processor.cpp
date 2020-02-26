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

#include "processor.h"

#include <cassert>

namespace uiiit {
namespace edge {

Processor::Processor(const std::string&  aName,
                     const ProcessorType aType,
                     const float         aSpeed,
                     const size_t        aCores,
                     const uint64_t      aMem)
    : theName(aName)
    , theType(aType)
    , theSpeed(aSpeed)
    , theCores(aCores)
    , theMemTotal(aMem)
    , theMemUsed(0)
    , theRunning(0)
    , theChrono(true)
    , theBusyChrono(false)
    , theBusyTime(0)
    , theLoad10Wnd(10)
    , theLoad30Wnd(30) {
  if (aName.empty()) {
    throw std::runtime_error("Invalid empty name for a processor");
  }
  if (aSpeed <= 0.0f) {
    throw std::runtime_error("Invalid negative speed for a processor");
  }
  if (aCores == 0) {
    throw std::runtime_error("Invalid 0 cores for a processor");
  }
  if (aMem == 0) {
    throw std::runtime_error("Invalid 0 memory available for a processor");
  }
}

void Processor::allocate(const uint64_t aSize) {
  if (aSize > memAvailable()) {
    throw std::runtime_error("Cannot allocate " + std::to_string(aSize) +
                             " bytes, " + std::to_string(memAvailable()) +
                             " bytes available");
  }
  theBusyTime += theBusyChrono.restart() * theRunning;
  theMemUsed += aSize;
  theRunning++;
}

void Processor::free(const uint64_t aSize) {
  if (aSize > memUsed()) {
    throw std::runtime_error("Cannot free " + std::to_string(aSize) +
                             " bytes, " + std::to_string(memUsed()) +
                             " bytes used");
  }
  theBusyTime += theBusyChrono.restart() * theRunning;
  theMemUsed -= aSize;
  theRunning--;
}

double Processor::opsToTime(const uint64_t aOps) const noexcept {
  return theRunning == 0 ? 0 : (aOps / equivalentSpeed());
}

double Processor::opsToTimePlusOne(const uint64_t aOps) const noexcept {
  const auto myRunning = theRunning + 1;
  return aOps / (std::min(theCores, myRunning) * theSpeed / myRunning);
}

uint64_t Processor::timeToOps(const double aTime) const noexcept {
  return theRunning == 0 ? 0 : (0.5 + aTime * equivalentSpeed());
}

double Processor::utilization() {
  const auto myElapsed = theChrono.restart();
  theBusyTime += theBusyChrono.restart() * theRunning;
  assert(myElapsed > 0);
  const auto myUtil = std::min(theBusyTime / (theCores * myElapsed), 1.0);
  theBusyTime       = 0;
  theLoad10Wnd.add(myUtil); // save in the moving average window
  theLoad30Wnd.add(myUtil); // save in the moving average window
  return myUtil;
}

std::array<double, 3> Processor::lastUtils() const noexcept {
  if (theLoad10Wnd.empty() or theLoad30Wnd.empty()) {
    return {{0.0, 0.0, 0.0}};
  }
  try {
    return {
        {theLoad30Wnd.last(), theLoad10Wnd.average(), theLoad30Wnd.average()}};
  } catch (...) {
  }
  // this should not happen because we have checked beforehand if the moving
  // windows are empty
  assert(false);
  return {{0.0, 0.0, 0.0}};
}

double Processor::equivalentSpeed() const noexcept {
  assert(theRunning > 0);
  return std::min(theCores, theRunning) * theSpeed / theRunning;
}

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                 aStream,
                         const uiiit::edge::Processor& aProcessor) {
  aStream << "name " << aProcessor.name() << ", "
          << "type " << aProcessor.type() << ", " << aProcessor.cores()
          << " cores, "
          << "speed " << aProcessor.speed() << " operations/s per core, "
          << "memory total/available/used " << aProcessor.memTotal() << '/'
          << aProcessor.memAvailable() << '/' << aProcessor.memUsed()
          << " bytes, " << aProcessor.running() << " running tasks";
  return aStream;
}
