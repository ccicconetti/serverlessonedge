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

#include "Edge/Model/chain.h"
#include "Support/chrono.h"
#include "Support/conf.h"
#include "Support/macros.h"
#include "Support/queue.h"
#include "Support/random.h"
#include "Support/stat.h"

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace uiiit {

namespace support {
class Saver;
class SummaryStat;
} // namespace support

namespace edge {
class EdgeClientInterface;
struct State;
struct LambdaResponse;
} // namespace edge

namespace simulation {

struct Stats {};

/**
 * Interface for edge clients that can be run in a ThreadPool.
 *
 * Unless modified after construction, the lambda request contains a JSON
 * structure with a single field 'input' whose value is equal to a sequence
 * of 'A' characters so as to match the intended size, depending on the
 * specialized class.
 *
 * The lambda request can be overridden after construction and set to a custom
 * content.
 *
 * The client can operate in the following modes:
 *
 * - single function execution: in this case the lambda name must be non-empty
 *   and the client invokes at each loop iteraiont a single function call,
 *   measuring the response time;
 * - chain of functions: the lambda name is empty and the client invokes
 *   one function another another, also embedding the functions' states
 *   in each call depending on the state dependencies; in this mode the info
 *   about the chain is provided by a dedicated method setChain() which must
 *   be called before the client is started.
 */
class Client
{
 public:
  NONCOPYABLE_NONMOVABLE(Client);

  explicit Client(const size_t                 aSeedUser,
                  const size_t                 aSeedInc,
                  const size_t                 aNumRequests,
                  const std::set<std::string>& aServers,
                  const support::Conf&         aClientConf,
                  const std::string&           aLambda,
                  const support::Saver&        aSaver,
                  const bool                   aDry);

  /**
   * Set the lambda request to a custom content, overriding the value it
   * would have according to the specialized class.
   *
   * Set to an empty string to revert to the previous behavior.
   */
  void setContent(const std::string& aContent);

  /**
   * @brief Set the chain info for a client operating in function chain mode.
   *
   * @param aChain The chain of functions to be invoked and the state
   * dependencies.
   *
   * @param aStateSizes The size, in bytes, of the states.
   *
   * @throw std::runtime_error if aStateSizes does not include all the states
   * in aChain or if the client has been configured in the ctor to operate
   * in single function call mode
   */
  void setChain(const edge::model::Chain&            aChain,
                const std::map<std::string, size_t>& aStateSizes);

  //! Set the callback endpoint in outgoing requests.
  void setCallback(const std::string& aCallback);

  /**
   * @brief Set the end-point of the server to be used for remote states.
   *
   * Calling this method causes the state to be uploaded to the server
   * specified and then remote states to be used afterwards.
   *
   * @param aStateEndpoint the end-point of the local state server to be used.
   */
  void setStateServer(const std::string& aStateEndpoint);

  //! Draw size from a uniform r.v.
  void setSizeDist(const size_t aSizeMin, const size_t aSizeMax);

  //! Pick one of the given sizes randomly (repeated values ok).
  void setSizeDist(const std::vector<size_t>& aSizes);

  virtual ~Client();

  //! Called by the ThreadPool when started.
  void operator()();

  //! Stop the execution. Wait until all operations end.
  void stop();

  /**
   * \param aDuration the duration of the simulation, in seconds.
   *
   * \return the number of lambda requests that would be sent if the response
   * time of the server was 0.
   */
  virtual size_t simulate(const double aDuration) = 0;

  //! \return the latency summary statistics.
  const support::SummaryStat& latencyStat() const noexcept {
    return theLatencyStat;
  }

  //! \return the processing time summary statistics.
  const support::SummaryStat& processingStat() const noexcept {
    return theProcessingStat;
  }

  //! Record the statistics for a lambda response.
  void recordStat(const edge::LambdaResponse& aResponse);

 protected:
  /**
   * Send out a lambda request, wait for the response.
   *
   * \param aSize The lambda size, in octets.
   *
   * If the lambda request is successful then the latency is pushed to the saver
   * and to the summary statistics object.
   *
   * \throw std::runtime_error if the lambda returns an error.
   */
  void sendRequest(const size_t aSize);

  //! Sleep for the given amount of time, in fractional seconds.
  void sleep(const double aTime);

  //! \return the size of the next lambda request.
  size_t nextSize();

 private:
  /**
   * Execution loop.
   *
   * \return the number of requests sent (can be zero).
   */
  virtual size_t loop() = 0;

  //! Set the finish flag.
  void finish();

  /**
   * @brief Run a single function
   *
   * @return the function response
   */
  std::unique_ptr<edge::LambdaResponse>
  singleExecution(const std::string& aInput);

  /**
   * @brief Run a chain of function executions.
   *
   * @return the last function response
   * of the whole chain.
   */
  std::unique_ptr<edge::LambdaResponse>
  functionChain(const std::string& aInput);

  //! Prepare the states if not valid.
  void validateStates();

 protected:
  const size_t theSeedUser;
  const size_t theSeedInc;

 private:
  mutable std::mutex                         theMutex;
  std::condition_variable                    theSleepCondition;
  std::condition_variable                    theExitCondition;
  bool                                       theStopFlag;
  bool                                       theFinishedFlag;
  bool                                       theNotStartedFlag;
  support::Chrono                            theLambdaChrono;
  std::unique_ptr<edge::EdgeClientInterface> theClient;
  const size_t                               theNumRequests;
  const std::string                          theLambda;
  const support::Saver&                      theSaver;
  support::SummaryStat                       theLatencyStat;
  support::SummaryStat                       theProcessingStat;
  const bool                                 theDry;
  support::Queue<int>                        theSendRequestQueue;
  std::map<std::string, edge::State>         theLastStates;
  bool                                       theInvalidStates;

  // set in setSizeDist()
  std::unique_ptr<support::UniformIntRv<size_t>> theSizeDist;
  std::vector<size_t>                            theSizes;

  // set in setChain()
  std::unique_ptr<edge::model::Chain> theChain;
  std::map<std::string, size_t>       theStateSizes;

  // set in setCallback()
  std::string theCallback;

  // set in setContent()
  std::string theContent;

  // set in setStateServer()
  std::string theStateEndpoint;
};

} // namespace simulation
} // namespace uiiit
