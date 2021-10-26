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

#include "Edge/computer.h"
#include "Edge/edgeserver.h"
#include "Support/chrono.h"
#include "Support/queue.h"

namespace uiiit {

namespace support {
template <class OBJECT>
class ThreadPool;
}

namespace edge {

class EdgeComputer final : public EdgeServer
{
  /**
   * Descriptor for a pending lambda request.
   *
   * Created as a new lambda request arrives, destroyed immediately after the
   * lambda response is generated. A chronometer is started automatically upon
   * creation.
   */
  struct Descriptor {
    explicit Descriptor()
        : theCondition()
        , theResponse()
        , theDone(false)
        , theChrono(true) {
    }
    std::condition_variable               theCondition;
    std::shared_ptr<const LambdaResponse> theResponse;
    bool                                  theDone;
    support::Chrono                       theChrono;
  };

  using UtilCallback = Computer::UtilCallback;

  class AsyncWorker final
  {
   public:
    explicit AsyncWorker(EdgeComputer&                       aParent,
                         support::Queue<rpc::LambdaRequest>& aQueue);
    void operator()();
    void stop();

   private:
    EdgeComputer&                       theParent;
    support::Queue<rpc::LambdaRequest>& theQueue;
  };

 public:
  /**
   * Create an edge computer that support asynchronous calls.
   *
   * \param aNumThreads the number of threads needed for asynchronous responses.
   *
   * \param aServerEndpoint the listening end-point of this server.
   *
   * \param aCallback the function called as new load values are available.
   */
  explicit EdgeComputer(const size_t        aNumThreads,
                        const std::string&  aServerEndpoint,
                        const UtilCallback& aCallback);

  //! Create an edge computer that only supports synchronous calls.
  explicit EdgeComputer(const std::string&  aServerEndpoint,
                        const UtilCallback& aCallback);

  ~EdgeComputer() override;

  //! \return The computer inside this edge server.
  Computer& computer() {
    return theComputer;
  }

 private:
  //! Callback invoked by the computer once a task is complete.
  void taskDone(const uint64_t                               aId,
                const std::shared_ptr<const LambdaResponse>& aResponse);

  //! Perform actual processing of a lambda request.
  rpc::LambdaResponse process(const rpc::LambdaRequest& aReq) override;

  //! Execute a lambda function (blocks until done).
  rpc::LambdaResponse blockingExecution(const rpc::LambdaRequest& aReq);

 private:
  Computer                                        theComputer;
  std::condition_variable                         theDescriptorsCv;
  std::map<uint64_t, std::unique_ptr<Descriptor>> theDescriptors;

  // only for asynchronous responses (if num threads > 1)
  using WorkersPool = support::ThreadPool<std::unique_ptr<AsyncWorker>>;
  const std::unique_ptr<WorkersPool>                        theAsyncWorkers;
  const std::unique_ptr<support::Queue<rpc::LambdaRequest>> theAsyncQueue;
};

} // end namespace edge
} // end namespace uiiit
