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

#include <glog/logging.h>
#include <string>

#include <proxygen/httpclient/samples/curl/CurlClient.h>
#include <proxygen/httpserver/samples/hq/HQParams.h>
#include <proxygen/lib/http/session/HQUpstreamSession.h>

#include <quic/common/Timers.h>

#include <quic/client/QuicClientTransport.h>
#include <quic/congestion_control/CongestionControllerFactory.h>
#include <quic/fizz/client/handshake/FizzClientQuicHandshakeContext.h>

namespace uiiit {
namespace edge {

namespace qs = quic::samples;

using FizzClientContextPtr = std::shared_ptr<fizz::client::FizzClientContext>;

class EdgeClientQuic final
    : private proxygen::HQSession::ConnectCallback // , public
                                                   // EdgeClientInterface
{
 public:
  /**
   * \param aServerEndpoint the edge server
   */
  explicit EdgeClientQuic(const qs::HQParams& aQuicParamsConf);

  ~EdgeClientQuic(); // override;

  // LambdaResponse RunLambda(const LambdaRequest& aReq, const bool aDry)
  // override;

  void initializeQuicTransportClient();

  FizzClientContextPtr createFizzClientContext(const qs::HQParams& params);

  proxygen::HTTPTransaction* sendRequest(const proxygen::URL& requestUrl);

  void sendRequests(bool closeSession, uint64_t numOpenableStreams);

  // these 3 functions come from the pure virtual ones in ConnectCallback
  void connectSuccess() override;

  void onReplaySafe() override;

  void connectError(std::pair<quic::QuicErrorCode, std::string> error) override;

 private:
  const qs::HQParams&                        theQuicParamsConf;
  std::shared_ptr<quic::QuicClientTransport> quicClient_;
  quic::TimerHighRes::SharedPtr
      pacingTimer_; // verifica se nella initializeQuicClient puoi non settarlo
                    // in caso
  folly::EventBase                                    evb_;
  proxygen::HQUpstreamSession*                        session_;
  std::list<std::unique_ptr<CurlService::CurlClient>> curls_;
  std::deque<folly::StringPiece>                      httpPaths_;

}; // end class EdgeClient

} // end namespace edge
} // end namespace uiiit
