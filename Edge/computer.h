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

#include "Support/chrono.h"
#include "Support/macros.h"

#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>

namespace uiiit {
namespace edge {

class Processor;
class Container;
class Lambda;
enum class ProcessorType;
struct LambdaRequest;
struct LambdaResponse;
struct ContainerList;

struct InitDone : public std::runtime_error {
  explicit InitDone()
      : std::runtime_error("Configuration after initialization is complete") {
  }
};

struct DupProcessorName : public std::runtime_error {
  explicit DupProcessorName(const std::string& aComputer,
                            const std::string& aProcessor)
      : std::runtime_error("Duplicate processor " + aProcessor +
                           " added to computer " + aComputer) {
  }
};

struct NoProcessorFound : public std::runtime_error {
  explicit NoProcessorFound(const std::string& aComputer,
                            const std::string& aProcessor)
      : std::runtime_error("No processor " + aProcessor +
                           " found in computer " + aComputer) {
  }
};

struct DupContainerName : public std::runtime_error {
  explicit DupContainerName(const std::string& aComputer,
                            const std::string& aProcessor,
                            const std::string& aContainer)
      : std::runtime_error("Duplicate container " + aContainer +
                           " hosted by processor " + aProcessor +
                           " added to computer " + aComputer) {
  }
};

struct DupLambdaName : public std::runtime_error {
  explicit DupLambdaName(const std::string& aComputer,
                         const std::string& aLambda)
      : std::runtime_error("Duplicate lambda " + aLambda +
                           " added to computer " + aComputer) {
  }
};

struct NoContainerFound : public std::runtime_error {
  explicit NoContainerFound(const std::string& aComputer,
                            const std::string& aLambda)
      : std::runtime_error("No container for lambda " + aLambda +
                           " found in computer " + aComputer) {
  }
};

/**
 * Abstraction of a computer containing one or more processors and one or more
 * containers, each bound to only one processor.
 */
class Computer final
{
 public:
  NONCOPYABLE_NONMOVABLE(Computer);

  using Callback = std::function<void(
      const uint64_t                               aId,
      const std::shared_ptr<const LambdaResponse>& aResponse)>;

  using UtilCallback =
      std::function<void(const std::map<std::string, double>&)>;

  /**
   * Build an empty computer.
   * Processors and containers must be added using addProcessor() and
   * addContaier() prior to any call to the addTask() method.
   *
   * \param aName The computer name.
   * \param aCallback The function called upon return of a lambda.
   * \param aUtilCallback The function called periodically to report the
   * utilization of all processors. If empty, then the thread that collects the
   * utilization is not started.
   *
   * \throw std::runtime_error if the callback is not callable.
   */
  explicit Computer(const std::string&  aName,
                    const Callback&     aCallback,
                    const UtilCallback& aUtilCallback);

  ~Computer();

  /**
   * Add a new processor.
   *
   * \param aName The processor name.
   * \param aType The processor type.
   * \param aSpeed The speed, in number of operations per second, per core.
   * \param aCores The number of cores.
   * \param aMem The memory availability, in bytes.
   *
   * \throws DupProcessorName if there is already another processor with the
   * same name.
   *
   * \throw InitDone if this method is called once the initialized is complete.
   */
  void addProcessor(const std::string&  aName,
                    const ProcessorType aType,
                    const float         aSpeed,
                    const size_t        aCores,
                    const uint64_t      aMem);

  /**
   * Add a container.
   *
   * \param aName The container name.
   * \param aProcName The processor name.
   * \param aLambda The function that this container executes.
   * \param aNumWorkers The number of available workers. This value limits the
   * concurrency level within the container.
   *
   * \throw DupContainerName if there is already another container with the
   * same name
   *
   * \throw NoProcessorFound if there is no processor with the given name.
   *
   * \throw DupLambdaName if a container with with the same lambda as another
   * one already present is added.
   *
   * \throw InitDone if this method is called once the initialized is complete.
   */
  void addContainer(const std::string& aName,
                    const std::string& aProcName,
                    const Lambda&      aLambda,
                    const size_t       aNumWorkers);

  /**
   * Add a new task to the computer.
   * This method is asynchronous: it returns immediately after scheduling the
   * task for execution. When the task execution is complete, the callback
   * specified in the ctor will be called.
   *
   * \param aRequest The lambda request to be executed.
   *
   * \return The identifier of the request, that will allow the recipient of the
   * callback to identify which task has been actually completed.
   *
   * \throw NoContainerFound if there is no matching container for this request.
   */
  uint64_t addTask(const LambdaRequest& aRequest);

  /**
   * Simulate the execution of the task requested, assuming that no other tasks
   * with be added until this one is finished.
   *
   * \param aRequest the lambda request to be simulated.
   *
   * \param aLastUtils the last 1, 10, and 30 seconds last loads.
   *
   * \return the time required to execute the lambda on this computer.
   */
  double simTask(const LambdaRequest&   aRequest,
                 std::array<double, 3>& aLastUtils);

  //! \return The computer's name.
  const std::string& name() const noexcept {
    return theName;
  }

  //! \return the list of containers.
  std::shared_ptr<ContainerList> containerList() const;

  // printers
  void printProcessors(std::ostream& aStream) const;
  void printContainers(std::ostream& aStream) const;

 private:
  //! Thread dispatching tasks.
  void dispatcher();
  //! Thread collecting processors' utilizations.
  void utilCollector();

  //! \throw InitDone if addTask() has been already called.
  void throwIfInitDone() const;
  //! Advance clocks of all containers and stop the system timer.
  void pause();
  //! Restart the system timer if there are active tasks.
  void resume();
  //! \return true if there is at least one task active.
  bool someActive() const;
  //! \return true if there is at least one task completed.
  bool someReady() const;
  //! Dispatch all tasks whose execution is finished.
  void dispatchCompletedTasks();

 private:
  const std::string  theName;
  const Callback     theCallback;
  const UtilCallback theUtilCallback;

  mutable std::mutex      theMutex;
  std::condition_variable theCondition;
  std::condition_variable theUtilCondition;
  bool                    theInitDone;
  bool                    theTerminating;
  bool                    theNewTask;
  uint64_t                theNextId;
  support::Chrono         theChrono;
  std::thread             theDispatcher;
  std::thread             theUtilCollector;

  std::map<std::string, std::unique_ptr<Processor>> theProcessors;
  std::set<std::string>                             theContainerNames;
  std::map<std::string, std::unique_ptr<Container>> theContainers;
};

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                aStream,
                         const uiiit::edge::Computer& aComputer);
