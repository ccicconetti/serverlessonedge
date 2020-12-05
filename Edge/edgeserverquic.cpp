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

#include "edgeserverquic.h"

#include <glog/logging.h>

#include <fizz/server/AeadTicketCipher.h>
#include <fizz/server/CertManager.h>
#include <fizz/server/TicketCodec.h>

#include <boost/algorithm/string.hpp>

#include <folly/io/async/EventBaseManager.h>

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/HTTPTransactionHandlerAdaptor.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>

#include <proxygen/httpserver/samples/hq/FizzContext.h>
#include <proxygen/httpserver/samples/hq/HQLoggerHelper.h>
#include <proxygen/httpserver/samples/hq/HQParams.h>
//#include <proxygen/httpserver/samples/hq/SampleHandlers.h>

#include <proxygen/lib/http/session/HQDownstreamSession.h>
#include <proxygen/lib/http/session/HTTPSessionController.h>
#include <proxygen/lib/utils/WheelTimerInstance.h>

#include <quic/congestion_control/ServerCongestionControllerFactory.h>
#include <quic/logging/FileQLogger.h>
#include <quic/server/QuicServer.h>
#include <quic/server/QuicServerTransport.h>
#include <quic/server/QuicSharedUDPSocketFactory.h>

namespace uiiit {
namespace edge {

using namespace std::chrono_literals;

//*****************************************************************
proxygen::HTTPTransactionHandler*
Dispatcher::getRequestHandler(proxygen::HTTPMessage* msg,
                              const qs::HQParams&    params) {
  DCHECK(msg);
  auto path = msg->getPathAsStringPiece();
  if (path == "/" || path == "/echo") {
    return new qs::EchoHandler(params);
  }
  return new qs::DummyHandler(params);
}

//******************************************************************
HQSessionController::HQSessionController(
    const qs::HQParams&                   params,
    const HTTPTransactionHandlerProvider& httpTransactionHandlerProvider)
    : params_(params)
    , httpTransactionHandlerProvider_(httpTransactionHandlerProvider) {
}

proxygen::HQSession* HQSessionController::createSession() {
  wangle::TransportInfo tinfo;
  session_ =
      new proxygen::HQDownstreamSession(params_.txnTimeout, this, tinfo, this);
  return session_;
}

void HQSessionController::startSession(std::shared_ptr<quic::QuicSocket> sock) {
  CHECK(session_);
  // session_->setSocket(std::move(sock));
  // session_->startNow();
  LOG(INFO) << "HQSessionController::startSession\n";
}

void HQSessionController::onTransportReady(
    proxygen::HTTPSessionBase* /*session*/) {
  LOG(INFO) << "HQSessionController::onTransportReady\n";
  // if (params_.sendKnobFrame) {
  //   sendKnobFrame("Hello, World from Server!");
  // }
}

void HQSessionController::onDestroy(const proxygen::HTTPSessionBase&) {
  LOG(INFO) << "HQSessionController::onDestroy\n";
}

// void HQSessionController::sendKnobFrame(const folly::StringPiece str) {
//   if (str.empty()) {
//     return;
//   }
//   uint64_t knobSpace = 0xfaceb00c;
//   uint64_t knobId    = 200;
//   Buf      buf(folly::IOBuf::create(str.size()));
//   memcpy(buf->writableData(), str.data(), str.size());
//   buf->append(str.size());
//   VLOG(10) << "Sending Knob Frame to peer. KnobSpace: " << std::hex <<
//   knobSpace
//            << " KnobId: " << std::dec << knobId << " Knob Blob" << str;
//   const auto knobSent = session_->sendKnob(0xfaceb00c, 200, std::move(buf));
//   if (knobSent.hasError()) {
//     LOG(ERROR) << "Failed to send Knob frame to peer. Received error: "
//                << knobSent.error();
//   }
// }

proxygen::HTTPTransactionHandler*
HQSessionController::getRequestHandler(proxygen::HTTPTransaction& /*txn*/,
                                       proxygen::HTTPMessage* msg) {
  LOG(INFO) << "HQSessionController::getRequestHandler\n";
  return httpTransactionHandlerProvider_(msg, params_);
}

proxygen::HTTPTransactionHandler* FOLLY_NULLABLE
HQSessionController::getParseErrorHandler(
    proxygen::HTTPTransaction* /*txn*/,
    const proxygen::HTTPException& /*error*/,
    const folly::SocketAddress& /*localAddress*/) {
  LOG(INFO) << "HQSessionController::getParseErrorHandler\n";
  return nullptr;
}

proxygen::HTTPTransactionHandler* FOLLY_NULLABLE
HQSessionController::getTransactionTimeoutHandler(
    proxygen::HTTPTransaction* /*txn*/,
    const folly::SocketAddress& /*localAddress*/) {
  LOG(INFO) << "HQSessionController::getTransactionTimeoutHandler\n";
  return nullptr;
}

void HQSessionController::attachSession(
    proxygen::HTTPSessionBase* /*session*/) {
  LOG(INFO) << "HQSessionController::attachSession\n";
}

void HQSessionController::detachSession(
    const proxygen::HTTPSessionBase* /*session*/) {
  LOG(INFO) << "HQSessionController::detachSession\n";
  delete this;
}

//****************************************************************

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

HQServer::HQServer(
    const qs::HQParams&            params,
    HTTPTransactionHandlerProvider httpTransactionHandlerProvider)
    : params_(params)
    , server_(quic::QuicServer::createQuicServer()) {
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

//*****************************************************************

H2Server::SampleHandlerFactory::SampleHandlerFactory(
    const qs::HQParams&            params,
    HTTPTransactionHandlerProvider httpTransactionHandlerProvider)
    : params_(params)
    , httpTransactionHandlerProvider_(
          std::move(httpTransactionHandlerProvider)) {
}

H2Server::SampleHandlerFactory::~SampleHandlerFactory() {
}

void H2Server::SampleHandlerFactory::onServerStart(
    folly::EventBase* /*evb*/) noexcept {
  LOG(INFO) << "H2Server::onServerStart\n";
}

void H2Server::SampleHandlerFactory::onServerStop() noexcept {
  LOG(INFO) << "H2Server::onServerStop\n";
}

proxygen::RequestHandler*
H2Server::SampleHandlerFactory::onRequest(proxygen::RequestHandler*,
                                          proxygen::HTTPMessage* msg) noexcept {
  LOG(INFO) << "H2Server::onRequest\n";
  return new proxygen::HTTPTransactionHandlerAdaptor(
      httpTransactionHandlerProvider_(msg, params_));
}

std::unique_ptr<proxygen::HTTPServerOptions> H2Server::createServerOptions(
    const qs::HQParams&            params,
    HTTPTransactionHandlerProvider httpTransactionHandlerProvider) {

  auto serverOptions = std::make_unique<proxygen::HTTPServerOptions>();

  serverOptions->threads     = params.httpServerThreads;
  serverOptions->idleTimeout = params.httpServerIdleTimeout;
  serverOptions->shutdownOn  = params.httpServerShutdownOn;
  serverOptions->enableContentCompression =
      params.httpServerEnableContentCompression;
  serverOptions->initialReceiveWindow =
      params.transportSettings.advertisedInitialBidiLocalStreamWindowSize;
  serverOptions->receiveStreamWindowSize =
      params.transportSettings.advertisedInitialBidiLocalStreamWindowSize;
  serverOptions->receiveSessionWindowSize =
      params.transportSettings.advertisedInitialConnectionWindowSize;
  serverOptions->handlerFactories =
      proxygen::RequestHandlerChain()
          .addThen<SampleHandlerFactory>(
              params, std::move(httpTransactionHandlerProvider))
          .build();
  return serverOptions;
}

std::unique_ptr<H2Server::AcceptorConfig>
H2Server::createServerAcceptorConfig(const qs::HQParams& params) {
  auto acceptorConfig = std::make_unique<AcceptorConfig>();
  proxygen::HTTPServer::IPConfig ipConfig(
      params.localH2Address.value(), proxygen::HTTPServer::Protocol::HTTP2);
  ipConfig.sslConfigs.emplace_back(uiiit::edge::createSSLContext(params));
  acceptorConfig->push_back(ipConfig);
  return acceptorConfig;
}

std::thread
H2Server::run(const qs::HQParams&            params,
              HTTPTransactionHandlerProvider httpTransactionHandlerProvider) {

  // Start HTTPServer mainloop in a separate thread
  std::thread t([params = folly::copy(params),
                 httpTransactionHandlerProvider =
                     std::move(httpTransactionHandlerProvider)]() mutable {
    {
      auto acceptorConfig = createServerAcceptorConfig(params);
      auto serverOptions  = createServerOptions(
          params, std::move(httpTransactionHandlerProvider));

      //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      proxygen::HTTPServer server(std::move(*serverOptions));

      //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      // server.bind(std::move(*acceptorConfig));
      // server.start();
    }
    // HTTPServer traps the SIGINT.  resignal HQServer
    raise(SIGINT);
  });

  return t;
}

//*******************************************************************

/*
1) theMutex serve sempre in EdgeServerGrpc???

2) Valutare se è necessario lasciare tra i membri privati
theMutex, theServerEndpoint, theNumThreads
*/

EdgeServerQuic::EdgeServerQuic(EdgeServer&        aEdgeServer,
                               const qs::HQParams aQuicParamsConf)
    : EdgeServerImpl(aEdgeServer)
    , theMutex()
    , theServerEndpoint(aQuicParamsConf.host)
    , theNumThreads(aQuicParamsConf.httpServerThreads)
    , theHandlers()
    , theQuicParamsConf(aQuicParamsConf)
    , theQuicTransportServer(theQuicParamsConf, Dispatcher::getRequestHandler) {
}

void EdgeServerQuic::run() { // starts the server (it cannot be configured
                             // anymore)
  // H2Server::run() <- vedi come passare parametri e getRequestHandler
  // theQuicTranportServer.start();
  // theQuicTranportServer.getAddress(); // per attendere sia inizializzato
  // theQuicTranportServer.run();

  theEdgeServer.init();
}

void EdgeServerQuic::wait() { // wait for the server termination
  // h2server.join(); <- h2server fuori scope => membro?
}

EdgeServerQuic::~EdgeServerQuic() {
}

rpc::LambdaResponse EdgeServerQuic::process(const rpc::LambdaRequest& aReq) {
  return theEdgeServer.process(aReq);
}

std::set<std::thread::id> EdgeServerQuic::threadIds() const {
  std::set<std::thread::id> ret;
  for (const auto& myThread : theHandlers) {
    ret.insert(myThread.get_id());
  }
  return ret;
}

//**********************************************************************
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
      fizz::server::TicketCodec<fizz::server::CertificateStorage::X509>>>();
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

wangle::SSLContextConfig createSSLContext(const qs::HQParams& params) {
  wangle::SSLContextConfig sslCfg;
  sslCfg.isDefault          = true;
  sslCfg.clientVerification = folly::SSLContext::SSLVerifyPeerEnum::VERIFY;
  if (!params.certificateFilePath.empty() && !params.keyFilePath.empty()) {
    sslCfg.setCertificate(params.certificateFilePath, params.keyFilePath, "");
  } else {
    sslCfg.setCertificateBuf(kDefaultCertData, kDefaultKeyData);
  }
  sslCfg.setNextProtocols({"h2"});
  return sslCfg;
}
} // namespace edge
} // namespace uiiit
