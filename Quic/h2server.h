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

#include "Edge/edgeserverimpl.h"
#include "Quic/quicparams.h"
#include "Support/macros.h"

#include <proxygen/httpserver/HTTPServer.h>

#include <cassert>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>

namespace uiiit {
namespace edge {

using HTTPTransactionHandlerProvider =
    std::function<proxygen::HTTPTransactionHandler*(proxygen::HTTPMessage*,
                                                    const HQParams&)>;

class H2Server
{
  class SampleHandlerFactory : public proxygen::RequestHandlerFactory
  {
   public:
    explicit SampleHandlerFactory(
        const HQParams&                aQuicParamsConf,
        HTTPTransactionHandlerProvider aHttpTransactionHandlerProvider);

    virtual ~SampleHandlerFactory();

    void onServerStart(folly::EventBase* /*evb*/) noexcept override;

    void onServerStop() noexcept override;

    proxygen::RequestHandler*
    onRequest(proxygen::RequestHandler* /* handler */,
              proxygen::HTTPMessage* /* msg */) noexcept override;

   private:
    const HQParams&                theQuicParamsConf;
    HTTPTransactionHandlerProvider theHttpTransactionHandlerProvider;
  }; // SampleHandlerFactory

 public:
  static std::unique_ptr<proxygen::HTTPServerOptions> createServerOptions(
      const HQParams& /* params */,
      HTTPTransactionHandlerProvider aHttpTransactionHandlerProvider);

  using AcceptorConfig = std::vector<proxygen::HTTPServer::IPConfig>;

  static std::unique_ptr<AcceptorConfig>
  createServerAcceptorConfig(const HQParams& /* params */);

  // Starts H2 server in a background thread
  static std::thread
  run(const HQParams&                aQuicParamsConf,
      HTTPTransactionHandlerProvider aHttpTransactionHandlerProvider);

}; // class H2Server

wangle::SSLContextConfig createSSLContext(const HQParams& aQuicParamsConf);

} // namespace edge
} // namespace uiiit