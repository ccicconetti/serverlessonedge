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

#include "Edge/edgeclientinterface.h"
#include "Edge/edgemessages.h"
#include "RpcSupport/simpleclient.h"
#include "Support/conf.h"
#include "Support/queue.h"

#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace uiiit {
namespace edge {

// forward declaration
class EdgeClient;

/**
 * An edge client that has multiple possible destinations.
 *
 * Objects of this class spawn N+1 threads, where N is the number of
 * destinations.
 *
 * Each of the N threads contains an EdgeClient that performs lambda requests
 * as dispatched by the calling thread.
 *
 * The +1 thread does the clean-up after the first EdgeClient has returned,
 * which makes the calling thread move on. The calling thread blocks if this
 * thread is still waiting for still-pending previous lambda requests.
 */
class EdgeClientMulti final : public EdgeClientInterface
{
  //! Message-passing structure from main thread to client threads.
  struct MessageIn {
    MOVEONLY(MessageIn);

    //! Empty message for terminating.
    MessageIn()
        : theRequest(nullptr)
        , theDry(false) {
      // nihil
    }

    /**
     * \param aRequest pointer to the lambda request.
     * \param aDry true if the lambda is not to be actually executed.
     *
     * \pre aRequest is not null.
     */
    MessageIn(const LambdaRequest* aRequest, const bool aDry)
        : theRequest(aRequest)
        , theDry(aDry) {
      assert(aRequest != nullptr);
    }

    operator bool() const noexcept {
      return theRequest != nullptr;
    }

    const LambdaRequest* theRequest; // nullptr means terminating
    bool                 theDry;
  };

  //! Message-passing structure from client threads to main thread.
  struct MessageOut {
    MOVEONLY(MessageOut);

    //! Empty message for terminating.
    MessageOut(const size_t aIndex)
        : theIndex(aIndex)
        , theResponse(nullptr) {
    }

    /**
     * \param aIndex The index of the client returning the response.
     * \param aResponse The response returned.
     *
     * \pre aResponse is not null.
     */
    MessageOut(const size_t aIndex, std::unique_ptr<LambdaResponse>&& aResponse)
        : theIndex(aIndex)
        , theResponse(std::move(aResponse)) {
      assert(theResponse);
    }

    operator bool() const noexcept {
      return theResponse.get() != nullptr;
    }

    size_t                          theIndex;
    std::unique_ptr<LambdaResponse> theResponse; // nullptr means terminating
  };

  //! One per client connected.
  struct Desc {
    size_t                               theIndex;
    std::string                          theEndpoint;
    std::unique_ptr<EdgeClientInterface> theClient;
    support::Queue<MessageIn>            theQueueIn;
    std::thread                          theThread;
  };

 public:
  /**
   * \param aServerEndpoints the edge servers to use.
   *
   * \param aClientConf the edge client configuration
   * ("type=grpc,persistence=0.05" by default): "type" is
   * the edge client transport protocol, "persistence" is instead the
   * p-persistence probability to probe another destination
   *
   * \pre aPersistenceProb is in [0, 1]
   */
  explicit EdgeClientMulti(const std::set<std::string>& aServerEndpoints,
                           const support::Conf&         aClientConf);

  ~EdgeClientMulti() override;

  /**
   * Execution of a lambda with given input is always done towards the primary
   * destination. Additionaly, depending on the policy used, it may also be sent
   * to other secondary executors.
   *
   * This function returns after the first of the lambda responses is
   * received. However, if the same function is called again, it blocks
   * until the last of the lambda responses is received.
   *
   * If a connection error occurs or the executor replies with an
   * error code, the response is discarded if there are other pending
   * requests. An error lambda response is only returned if all the
   * selected destinations, in addition to the primary, fail.
   *
   * Failed destinations are not removed from the pool.
   */
  LambdaResponse RunLambda(const LambdaRequest& aReq, const bool aDry) override;

 private:
  //! \return the smoothing factor for delay computation
  static constexpr float alpha() {
    return 0.05;
  }

  //! Consumer body. When terminating returns false.
  bool consume();

  /**
   * Lambda execution body.
   *
   * If waits for an incoming message in the Desc::theQueueIn, then try
   * to execute the lambda via Desc::theClient; if something fails it
   * pushes to Desc::theQueueOut an empty pointer, otherwise a newly
   * created LambdaResponse object.
   *
   * \param aDesc The descriptor to be used by this thread.
   *
   * \return False when terminating, ie. Desc::theQueueIn is closed.
   */
  bool execLambda(Desc& aDesc);

  //! \return the set of other destinations to be reached.
  std::set<size_t> secondary();

 private:
  const float thePersistenceProb;

  std::vector<Desc>                    theDesc; // never modified after ctor
  std::thread                          theConsumer;
  support::Queue<std::set<size_t>>     theConsumerQueue;
  support::Queue<bool>                 theCallingQueue;
  support::Queue<MessageOut>           theQueueOut;
  std::unique_ptr<const LambdaRequest> thePendingRequest;
  size_t                               thePrimary;
}; // end class EdgeClientMulti

} // end namespace edge
} // end namespace uiiit