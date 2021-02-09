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
#include "Support/conf.h"
#include "Support/macros.h"
#include "Support/random.h"
#include "Support/stat.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

namespace uiiit {

namespace support {
class Saver;
class SummaryStat;
} // namespace support

namespace edge {
class EdgeClientInterface;
}

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

 protected:
  const size_t theSeedUser;
  const size_t theSeedInc;

 private:
  mutable std::mutex                         theMutex;
  std::condition_variable                    theSleepCondition;
  std::condition_variable                    theExitCondition;
  bool                                       theStopFlag;
  bool                                       theFinishedFlag;
  support::Chrono                            theLambdaChrono;
  std::unique_ptr<edge::EdgeClientInterface> theClient;
  const size_t                               theNumRequests;
  const std::string                          theLambda;
  const support::Saver&                      theSaver;
  support::SummaryStat                       theLatencyStat;
  support::SummaryStat                       theProcessingStat;
  const bool                                 theDry;

  // set in setSizeDist()
  std::unique_ptr<support::UniformIntRv<size_t>> theSizeDist;
  std::vector<size_t>                            theSizes;

  // set in setContent()
  std::string theContent;
};

} // namespace simulation
} // namespace uiiit
