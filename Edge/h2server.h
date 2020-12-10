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

#include "Support/macros.h"

#include <cassert>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>

#include "edgeserverimpl.h"
#include <proxygen/httpserver/samples/hq/HQParams.h>
#include <proxygen/httpserver/samples/hq/HQServer.h>

namespace uiiit {
namespace edge {

namespace qs = quic::samples;

using HTTPTransactionHandlerProvider =
    std::function<proxygen::HTTPTransactionHandler*(proxygen::HTTPMessage*,
                                                    const qs::HQParams&)>;

class H2Server
{
  class SampleHandlerFactory : public proxygen::RequestHandlerFactory
  {
   public:
    explicit SampleHandlerFactory(
        const qs::HQParams&            params,
        HTTPTransactionHandlerProvider httpTransactionHandlerProvider);

    virtual ~SampleHandlerFactory();

    void onServerStart(folly::EventBase* /*evb*/) noexcept override;

    void onServerStop() noexcept override;

    proxygen::RequestHandler*
    onRequest(proxygen::RequestHandler* /* handler */,
              proxygen::HTTPMessage* /* msg */) noexcept override;

   private:
    const qs::HQParams&            params_;
    HTTPTransactionHandlerProvider httpTransactionHandlerProvider_;
  }; // SampleHandlerFactory

 public:
  static std::unique_ptr<proxygen::HTTPServerOptions> createServerOptions(
      const qs::HQParams& /* params */,
      HTTPTransactionHandlerProvider httpTransactionHandlerProvider);

  using AcceptorConfig = std::vector<proxygen::HTTPServer::IPConfig>;

  static std::unique_ptr<AcceptorConfig>
  createServerAcceptorConfig(const qs::HQParams& /* params */);

  // Starts H2 server in a background thread
  static std::thread
  run(const qs::HQParams&            params,
      HTTPTransactionHandlerProvider httpTransactionHandlerProvider);

}; // class H2Server

wangle::SSLContextConfig createSSLContext(const qs::HQParams& params);

} // namespace edge
} // namespace uiiit