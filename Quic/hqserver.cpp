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

#include "Quic/hqserver.h"
#include "Quic/edgeserverquic.h"
#include "Quic/hqservertransportfactory.h"

#include <fizz/server/AeadTicketCipher.h>
#include <fizz/server/CertManager.h>
#include <fizz/server/TicketCodec.h>

#include <quic/congestion_control/ServerCongestionControllerFactory.h>
#include <quic/server/QuicServerTransport.h>
#include <quic/server/QuicSharedUDPSocketFactory.h>

// namespace qs = quic::samples;

namespace uiiit {
namespace edge {

HQServer::HQServer(
    // const qs::HQParams&            params,
    const HQParams&                params,
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

// FizzServerContextPtr createFizzServerContext(const qs::HQParams& params) {
FizzServerContextPtr createFizzServerContext(const HQParams& params) {

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