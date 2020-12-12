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

#include "Quic/hqservertransportfactory.h"
#include "Quic/hqsessioncontroller.h"

namespace uiiit {
namespace edge {

HQServerTransportFactory::HQServerTransportFactory(
    const HQParams&                       params,
    const HTTPTransactionHandlerProvider& httpTransactionHandlerProvider)
    : params_(params)
    , httpTransactionHandlerProvider_(httpTransactionHandlerProvider) {
  LOG(INFO) << "HQServerTransportFactory CTOR\n";
}

quic::QuicServerTransport::Ptr HQServerTransportFactory::make(
    folly::EventBase*                      evb,
    std::unique_ptr<folly::AsyncUDPSocket> socket,
    const folly::SocketAddress& /* peerAddr */,
    std::shared_ptr<const fizz::server::FizzServerContext> ctx) noexcept {
  // Session controller is self owning
  auto hqSessionController =
      new HQSessionController(params_, httpTransactionHandlerProvider_);
  auto session = hqSessionController->createSession();
  CHECK_EQ(evb, socket->getEventBase());
  auto transport =
      quic::QuicServerTransport::make(evb, std::move(socket), *session, ctx);
  // if (!params_.qLoggerPath.empty()) {
  //   transport->setQLogger(std::make_shared<qs::HQLoggerHelper>(
  //       params_.qLoggerPath, params_.prettyJson,
  //       quic::VantagePoint::Server));
  // }
  hqSessionController->startSession(transport);
  return transport;
}

} // namespace edge
} // namespace uiiit