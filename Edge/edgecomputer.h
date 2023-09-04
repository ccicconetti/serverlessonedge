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

#include "Edge/edgeserver.h"
#include "Support/chrono.h"
#include "Support/queue.h"

#include <unordered_map>

namespace uiiit {

namespace support {
template <class OBJECT>
class ThreadPool;
}

namespace edge {

class EdgeClientGrpc;
class StateClient;

/**
 * @brief Edge server responding to lambda function invocations.
 *
 * Depending on the function invocation received it reacts differently:
 *
 * - if it is a dry request, then the function is not actually invoked but
 * an immediate reply is given with an estimate of the time it would require
 * to run the function in the current conditions, if available
 *
 * - if it is a synchronous request, then the function is invoked and
 * a reply is issued to the invoker
 *
 * - if it is an asynchronous function request, then an immediate response is
 * issued to the invoker, and then after invoking the function the real
 * response is sent to the callback specified in the request (which can be
 * different from the invoker)
 *
 * - if it is chain of function invocations, which are always asynchronous,
 * then an immediate response is issued to the invoker without invoking
 * the function; then, after invoking the function, if the current
 * function is the last in the chain then the response is sent to the callback
 * specified in the request, otherwise a new function is invoked on the
 * next computer through the companion endpoint
 *
 * - if it is a DAG workflow, then the behavior is the same as with a chain
 * of functions with two differences:
 *
 *   1) the invocation of the function is started only after all the
 *   invocations from the predecessors of the function have been received:
 *   if the current function has only one ingress in the DAG, then it
 *   behaves like a function chain; if it has N > 1 incoming edges in the DAG,
 *   then it skips the first (N-1) invocations received and then only
 *   handles the function invocation when receiving the N-th invocation
 *
 *   2) since a function can have > 1 outgoing edges in the DAG, after
 *   the invocation, it invokes all of them
 *
 * Specialized classes must define the methods to invoke a function (both
 * real invocation and dry run).
 */
class EdgeComputer : public EdgeServer
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
   * \param aSecure if true use SSL/TLS authentication.
   *
   * The companion end-point is empty by default. If needed, i.e., if this
   * edge computer is expected to serve function chains or DAGs, then it must be
   * set via the companion() method.
   */
  explicit EdgeComputer(const size_t       aNumThreads,
                        const std::string& aServerEndpoint,
                        const bool         aSecure);

  //! Create an edge computer that only supports synchronous calls.
  explicit EdgeComputer(const std::string& aServerEndpoint, const bool aSecure);

  ~EdgeComputer() override;

  /**
   * @brief Set the companion end-point, which is needed for chains/DAGs.
   *
   * @param aCompanionEndpoint the end-point of the destination to which to
   * send the next function invocation. If an empty string is passed then
   * the companion is cleared, i.e., the edge computer becomes synchronous
   * only again.
   *
   * Can be called multiple times, each time overring the previous value.
   *
   * @throw std::runtime_error if this edge computer is synchronous only.
   */
  void companion(const std::string& aCompanionEndpoint);

  /**
   * @brief Set the state end-point, which is needed for remote states.
   *
   * @param aStateEndpoint the end-point of the server that keeps the states
   * for this edge computer.

   * Can be called multiple times, each time overring the previous value.
   *
   * @throw std::runtime_error to disconnect from the current state server.
   */
  void state(const std::string& aStateEndpoint);

 protected:
  //! Callback invoked by the computer once a task is complete.
  void taskDone(const uint64_t                               aId,
                const std::shared_ptr<const LambdaResponse>& aResponse);

 private:
  //! Perform actual processing of a lambda request.
  rpc::LambdaResponse process(const rpc::LambdaRequest& aReq) override;

  //! Execute a lambda function (blocks until done).
  rpc::LambdaResponse blockingExecution(const rpc::LambdaRequest& aReq);

  /**
   * Starts the execution of a lambda function.
   *
   * @return a unique identifier of the task just started, which must be
   * terminated by calling taskDone().
   */
  virtual uint64_t realExecution(const rpc::LambdaRequest& aRequest) = 0;

  //! Estimate the time required to execute a lambda function and its load.
  virtual double dryExecution(const rpc::LambdaRequest& aRequest,
                              std::array<double, 3>&    aLastUtils) = 0;

  //! Return a client to access the local state server or throw.
  StateClient& stateClient() const;

  /**
   * @brief Handle remote states.
   *
   * Retrieve the remote states from the requests.
   * If the request contains a chain or DAG, then we only retrieve those
   * neeed by the current function. Otherwise, we retrieve all the
   * remote states.
   *
   * The remote states retrieved are then deleted from the original
   * location and the location is updated in the response.
   *
   * Embedded states are left unchanged.
   *
   * @param aRequest the lambda request.
   * @param aResponse the lamba response with modified states.
   *
   * @return true if all the states were retrieved with success.
   * @return false otherwise.
   */
  bool handleRemoteStates(const rpc::LambdaRequest& aRequest,
                          rpc::LambdaResponse&      aResponse) const;

  /**
   * @brief Check if the preconditions for running this request are satisfied.
   *
   * For chain requests the preconditions are automatically verified.
   *
   * For DAG workflows, a request can only be executed if all the invocations
   * expected have been received.
   *
   * @param aRequest the request to be checked.
   *
   * @return true if the request should be executed.
   * @return false if the e-computer should yield (= not executed the request).
   */
  bool checkPreconditions(const rpc::LambdaRequest& aRequest);

  //! @return a hash of a request.
  static std::string hash(const rpc::LambdaRequest& aRequest);

  /**
   * @brief Check if this is the terminating function.
   *
   * This can happen for three reasons:
   *
   * - single-function execution request
   * - last function in a chain
   * - last function in a DAG
   *
   * @return true if the request is the last invocation.
   */
  static bool lastFunction(const rpc::LambdaRequest& aRequest);

  //! @return a hash of a request.
  static std::string makeHash(const rpc::LambdaRequest& aRequest);

 private:
  const bool theSecure;

  std::condition_variable                         theDescriptorsCv;
  std::map<uint64_t, std::unique_ptr<Descriptor>> theDescriptors;

  // only for asynchronous responses (if num threads > 1)
  using WorkersPool = support::ThreadPool<std::unique_ptr<AsyncWorker>>;
  const std::unique_ptr<WorkersPool>                        theAsyncWorkers;
  const std::unique_ptr<support::Queue<rpc::LambdaRequest>> theAsyncQueue;

  // only for function chains and DAGs, which are asynchronous by default
  std::unique_ptr<EdgeClientGrpc> theCompanionClient;
  std::mutex                      theCompanionMutex;

  // only for remote states
  std::unique_ptr<StateClient> theStateClient;

  // only for DAGs
  // key:   a hash of the request
  // value: the number of invocations already received
  std::unordered_map<std::string, size_t> theInvocations;
};

} // end namespace edge
} // end namespace uiiit
