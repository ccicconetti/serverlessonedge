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

namespace uiiit {
namespace edge {

HQServer::HQServer(
    const HQParams&                aQuicParamsConf,
    HTTPTransactionHandlerProvider aHttpTransactionHandlerProvider)
    : theQuicParamsConf(aQuicParamsConf)
    , theQuicServer(quic::QuicServer::createQuicServer()) {
  VLOG(1) << "HQServer::ctor";
  theQuicServer->setBindV6Only(false);
  theQuicServer->setCongestionControllerFactory(
      std::make_shared<quic::ServerCongestionControllerFactory>());
  theQuicServer->setTransportSettings(theQuicParamsConf.transportSettings);
  theQuicServer->setCcpConfig(theQuicParamsConf.ccpConfig);
  theQuicServer->setQuicServerTransportFactory(
      std::make_unique<HQServerTransportFactory>(
          theQuicParamsConf, std::move(aHttpTransactionHandlerProvider)));
  theQuicServer->setQuicUDPSocketFactory(
      std::make_unique<quic::QuicSharedUDPSocketFactory>());
  theQuicServer->setHealthCheckToken("health");
  theQuicServer->setSupportedVersion(theQuicParamsConf.quicVersions);
  theQuicServer->setFizzContext(
      uiiit::edge::createFizzServerContext(theQuicParamsConf));
  if (theQuicParamsConf.rateLimitPerThread) {
    theQuicServer->setRateLimit(theQuicParamsConf.rateLimitPerThread.value(),
                                1s);
  }
}

void HQServer::start(size_t aNumberOfThreads) {
  VLOG(1) << "HQServer::start\n";
  theQuicServer->start(theQuicParamsConf.localAddress.value(),
                       aNumberOfThreads);
  theQuicServer->waitUntilInitialized(); // blocking, no race conditions
  const auto& boundAddr = theQuicServer->getAddress();
  LOG(INFO) << "EdgeServerQuic started at: " << boundAddr.describe();
} // namespace edge

void HQServer::stop() {
  VLOG(1) << "HQServer::stop";
  theQuicServer->shutdown();
  theEvb.terminateLoopSoon();
}

FizzServerContextPtr createFizzServerContext(const HQParams& aQuicParamsConf) {

  std::string myCertData = kDefaultCertData;
  std::string myKeyData  = kDefaultKeyData;
  auto        myCert     = fizz::CertUtils::makeSelfCert(myCertData, myKeyData);
  auto        myCertManager = std::make_unique<fizz::server::CertManager>();
  myCertManager->addCert(std::move(myCert), true);

  auto cert2 =
      fizz::CertUtils::makeSelfCert(kPrime256v1CertData, kPrime256v1KeyData);
  myCertManager->addCert(std::move(cert2), false);

  auto myServerCtx = std::make_shared<fizz::server::FizzServerContext>();
  myServerCtx->setCertManager(std::move(myCertManager));
  auto ticketCipher = std::make_shared<fizz::server::Aead128GCMTicketCipher<
      fizz::server::TicketCodec<fizz::server::CertificateStorage::X509>>>(
      myServerCtx->getFactoryPtr(), std::move(myCertManager));
  std::array<uint8_t, 32> ticketSeed;
  folly::Random::secureRandom(ticketSeed.data(), ticketSeed.size());
  ticketCipher->setTicketSecrets({{folly::range(ticketSeed)}});
  myServerCtx->setTicketCipher(ticketCipher);
  myServerCtx->setClientAuthMode(aQuicParamsConf.clientAuth);
  myServerCtx->setSupportedAlpns(aQuicParamsConf.supportedAlpns);
  myServerCtx->setRequireAlpn(true);
  myServerCtx->setSendNewSessionTicket(false);
  myServerCtx->setEarlyDataFbOnly(false);
  myServerCtx->setVersionFallbackEnabled(false);

  fizz::server::ClockSkewTolerance myTolerance;
  myTolerance.before = std::chrono::minutes(-5);
  myTolerance.after  = std::chrono::minutes(5);

  std::shared_ptr<fizz::server::ReplayCache> myReplayCache =
      std::make_shared<fizz::server::AllowAllReplayReplayCache>();

  myServerCtx->setEarlyDataSettings(
      true, myTolerance, std::move(myReplayCache));

  return myServerCtx;
}

void HQServer::run() {
  theEvb.loopForever();
}

} // namespace edge
} // namespace uiiit