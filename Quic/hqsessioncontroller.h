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

#include "Quic/quicparamsbuilder.h"

#include <proxygen/lib/http/session/HQSession.h>
#include <proxygen/lib/http/session/HTTPSession.h>
#include <proxygen/lib/http/session/HTTPSessionController.h>

namespace uiiit {
namespace edge {

using HTTPTransactionHandlerProvider =
    std::function<proxygen::HTTPTransactionHandler*(proxygen::HTTPMessage*,
                                                    const HQParams&)>;

class HQSessionController : public proxygen::HTTPSessionController,
                            proxygen::HTTPSession::InfoCallback
{
 public:
  using StreamData = std::pair<folly::IOBufQueue, bool>;

  explicit HQSessionController(const HQParams& /* params */,
                               const HTTPTransactionHandlerProvider&);

  ~HQSessionController() override = default;

  // Creates new HQDownstreamSession object, initialized with params_
  proxygen::HQSession* createSession();

  // Starts the newly created session. createSession must have been called.
  void startSession(std::shared_ptr<quic::QuicSocket> /* sock */);

  void onDestroy(const proxygen::HTTPSessionBase& /* session*/) override;

  proxygen::HTTPTransactionHandler*
  getRequestHandler(proxygen::HTTPTransaction& /*txn*/,
                    proxygen::HTTPMessage* /* msg */) override;

  proxygen::HTTPTransactionHandler*
  getParseErrorHandler(proxygen::HTTPTransaction* /*txn*/,
                       const proxygen::HTTPException& /*error*/,
                       const folly::SocketAddress& /*localAddress*/) override;

  proxygen::HTTPTransactionHandler* getTransactionTimeoutHandler(
      proxygen::HTTPTransaction* /*txn*/,
      const folly::SocketAddress& /*localAddress*/) override;

  void attachSession(proxygen::HTTPSessionBase* /*session*/) override;

  // The controller instance will be destroyed after this call.
  void detachSession(const proxygen::HTTPSessionBase* /*session*/) override;

  void onTransportReady(proxygen::HTTPSessionBase* /*session*/) override;
  void onTransportReady(const proxygen::HTTPSessionBase&) override {
  }

 private:
  // The owning session. NOTE: this must be a plain pointer to
  // avoid circular references
  proxygen::HQSession* theSession{nullptr};
  // Configuration params
  const HQParams& theQuicParamsConf;
  // Provider of HTTPTransactionHandler, owned by HQServerTransportFactory
  const HTTPTransactionHandlerProvider& theHttpTransactionHandlerProvider;
};

} // namespace edge
} // namespace uiiit