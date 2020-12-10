#include "hqserver.h"
#include "edgeserverquic.h"
#include "hqservertransportfactory.h"

#include <fizz/server/AeadTicketCipher.h>
#include <fizz/server/CertManager.h>
#include <fizz/server/TicketCodec.h>

#include <quic/congestion_control/ServerCongestionControllerFactory.h>
#include <quic/server/QuicServerTransport.h>
#include <quic/server/QuicSharedUDPSocketFactory.h>

namespace uiiit {
namespace edge {

HQServer::HQServer(
    const qs::HQParams&            params,
    HTTPTransactionHandlerProvider httpTransactionHandlerProvider)
    : params_(params)
    , server_(quic::QuicServer::createQuicServer()) {
  server_->setBindV6Only(false);
  server_->setCongestionControllerFactory(
      std::make_shared<quic::ServerCongestionControllerFactory>());
  server_->setTransportSettings(params_.transportSettings);
  server_->setCcpConfig(params_.ccpConfig);
  server_->setQuicServerTransportFactory(
      std::make_unique<HQServerTransportFactory>(
          params_, std::move(httpTransactionHandlerProvider)));
  server_->setQuicUDPSocketFactory(
      std::make_unique<quic::QuicSharedUDPSocketFactory>());
  server_->setHealthCheckToken("health");
  server_->setSupportedVersion(params_.quicVersions);
  server_->setFizzContext(uiiit::edge::createFizzServerContext(params_));
  if (params_.rateLimitPerThread) {
    server_->setRateLimit(params_.rateLimitPerThread.value(), 1s);
  }
}

std::thread HQServer::start() {
  LOG(INFO) << "HQServer::start\n";
  std::thread t = std::thread([this]() mutable {
    server_->start(params_.localAddress.value(),
                   5); //, std::thread::hardware_concurrency());

    server_->waitUntilInitialized();
    const auto& boundAddr = server_->getAddress();
    LOG(INFO) << "HQ server started at: " << boundAddr.describe();

    eventbase_.loopForever();
  });
  return t;
}

FizzServerContextPtr createFizzServerContext(const qs::HQParams& params) {

  std::string certData = kDefaultCertData;
  if (!params.certificateFilePath.empty()) {
    folly::readFile(params.certificateFilePath.c_str(), certData);
  }
  std::string keyData = kDefaultKeyData;
  if (!params.keyFilePath.empty()) {
    folly::readFile(params.keyFilePath.c_str(), keyData);
  }
  auto cert        = fizz::CertUtils::makeSelfCert(certData, keyData);
  auto certManager = std::make_unique<fizz::server::CertManager>();
  certManager->addCert(std::move(cert), true);

  auto cert2 =
      fizz::CertUtils::makeSelfCert(kPrime256v1CertData, kPrime256v1KeyData);
  certManager->addCert(std::move(cert2), false);

  auto serverCtx = std::make_shared<fizz::server::FizzServerContext>();
  serverCtx->setCertManager(std::move(certManager));
  auto ticketCipher = std::make_shared<fizz::server::Aead128GCMTicketCipher<
      fizz::server::TicketCodec<fizz::server::CertificateStorage::X509>>>(
      serverCtx->getFactoryPtr(), std::move(certManager));
  std::array<uint8_t, 32> ticketSeed;
  folly::Random::secureRandom(ticketSeed.data(), ticketSeed.size());
  ticketCipher->setTicketSecrets({{folly::range(ticketSeed)}});
  serverCtx->setTicketCipher(ticketCipher);
  serverCtx->setClientAuthMode(params.clientAuth);
  serverCtx->setSupportedAlpns(params.supportedAlpns);
  serverCtx->setRequireAlpn(true);
  serverCtx->setSendNewSessionTicket(false);
  serverCtx->setEarlyDataFbOnly(false);
  serverCtx->setVersionFallbackEnabled(false);

  fizz::server::ClockSkewTolerance tolerance;
  tolerance.before = std::chrono::minutes(-5);
  tolerance.after  = std::chrono::minutes(5);

  std::shared_ptr<fizz::server::ReplayCache> replayCache =
      std::make_shared<fizz::server::AllowAllReplayReplayCache>();

  serverCtx->setEarlyDataSettings(true, tolerance, std::move(replayCache));

  return serverCtx;
}

} // namespace edge
} // namespace uiiit