#include "hqservertransportfactory.h"
#include "hqsessioncontroller.h"

namespace uiiit {
namespace edge {

HQServerTransportFactory::HQServerTransportFactory(
    const qs::HQParams&                   params,
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