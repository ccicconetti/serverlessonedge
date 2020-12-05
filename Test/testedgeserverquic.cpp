#include "Edge/edgelambdaprocessoroptions.h"
#include "Edge/edgerouter.h"
#include "Edge/edgeserverimpl.h"
#include "Edge/edgeserverquic.h"

#include "gtest/gtest.h"
#include <glog/logging.h>

#include "Support/conf.h"

#include <folly/ssl/Init.h>
#include <proxygen/httpserver/samples/hq/HQParams.h>
#include <proxygen/httpserver/samples/hq/HQServer.h>

#include <proxygen/lib/http/SynchronizedLruQuicPskCache.h>
#include <proxygen/lib/http/session/HQSession.h>
#include <quic/QuicConstants.h>

namespace uiiit {
namespace edge {

struct TestEdgeServerQuic : public ::testing::Test {

  struct System {

    System(quic::samples::HQParams& myEdgeServerQuicParams) {
      // *** General Section ***
      myEdgeServerQuicParams.host         = "127.0.0.1";
      myEdgeServerQuicParams.port         = 6473;
      myEdgeServerQuicParams.mode         = quic::samples::HQMode::SERVER;
      myEdgeServerQuicParams.logprefix    = "server";
      myEdgeServerQuicParams.localAddress = folly::SocketAddress(
          myEdgeServerQuicParams.host, myEdgeServerQuicParams.port, true);
      // local_address deve rimanere vuoto, assegnabile solo in client mode
      myEdgeServerQuicParams.logdir      = "/tmp/logs";
      myEdgeServerQuicParams.logResponse = true;
      myEdgeServerQuicParams.outdir      = "";

      // *** TransportSettings ***
      myEdgeServerQuicParams.quicVersions = {
          quic::QuicVersion::MVFST,
          quic::QuicVersion::MVFST_D24,
          quic::QuicVersion::MVFST_EXPERIMENTAL,
          quic::QuicVersion::QUIC_DRAFT,
          quic::QuicVersion::QUIC_DRAFT_LEGACY};
      // draft_version = 0 di default
      // protocol = "" di default -> entro nell'else
      myEdgeServerQuicParams.supportedAlpns = {"h1q-fb",
                                               "h1q-fb-v2",
                                               proxygen::kH3FBCurrentDraft,
                                               proxygen::kH3CurrentDraft,
                                               proxygen::kH3LegacyDraft,
                                               proxygen::kHQCurrentDraft};
      myEdgeServerQuicParams.transportSettings
          .advertisedInitialConnectionWindowSize = 1024 * 1024 * 10;
      myEdgeServerQuicParams.transportSettings
          .advertisedInitialBidiLocalStreamWindowSize = 256 * 1024;
      myEdgeServerQuicParams.transportSettings
          .advertisedInitialBidiRemoteStreamWindowSize = 256 * 1024;
      myEdgeServerQuicParams.transportSettings
          .advertisedInitialUniStreamWindowSize    = 256 * 1024;
      myEdgeServerQuicParams.congestionControlName = "cubic";
      myEdgeServerQuicParams.congestionControl =
          quic::congestionControlStrToType("cubic");
      // since congestion control cannot be null since we are using default
      // values
      myEdgeServerQuicParams.transportSettings.defaultCongestionController =
          quic::CongestionControlType::Cubic;

      myEdgeServerQuicParams.transportSettings.maxRecvPacketSize =
          quic::kDefaultUDPReadBufferSize;
      myEdgeServerQuicParams.transportSettings.numGROBuffers_ =
          quic::kDefaultNumGROBuffers;
      myEdgeServerQuicParams.transportSettings.pacingEnabled = false;
      // pacingEnabled false di default, non entro nell'if e non setto
      // myEdgeServerQuicParams.transportSettings.pacingTimerTickInterval
      myEdgeServerQuicParams.transportSettings.batchingMode =
          quic::getQuicBatchingMode(static_cast<uint32_t>(
              quic::QuicBatchingMode::BATCHING_MODE_NONE));
      myEdgeServerQuicParams.transportSettings.useThreadLocalBatching = false;
      myEdgeServerQuicParams.transportSettings.threadLocalDelay =
          std::chrono::microseconds(1000);
      myEdgeServerQuicParams.transportSettings.maxBatchSize =
          quic::kDefaultQuicMaxBatchSize;
      myEdgeServerQuicParams.transportSettings.turnoffPMTUD = true;
      myEdgeServerQuicParams.transportSettings.partialReliabilityEnabled =
          false;
      // mode = HQMode::SERVER so we do not set
      // myEdgeServerQuicParams.transportSettings.shouldDrain
      // myEdgeServerQuicParams.transportSettings.attemptEarlyData
      myEdgeServerQuicParams.transportSettings.connectUDP = false;
      myEdgeServerQuicParams.transportSettings.maxCwndInMss =
          quic::kLargeMaxCwndInMss;
      myEdgeServerQuicParams.transportSettings.disableMigration = false;
      /*
       FLAGS_use_inplace_write = false default, so we do not enter in the if
       branch and we do not set
       myEdgeServerQuicParams.transportSettings.dataPathType
      */
      // FLAGS_rate_limit = -1 default so we do not set
      // myEdgeServerQuicParams.rateLimitPerThread
      myEdgeServerQuicParams.connectTimeout = std::chrono::milliseconds(2000);
      myEdgeServerQuicParams.ccpConfig      = "";
      myEdgeServerQuicParams.sendKnobFrame  = false;
      // sendKnobFrame default is false, so we do not enter in if branch
      myEdgeServerQuicParams.transportSettings.d6dConfig.enabled = false;
      myEdgeServerQuicParams.transportSettings.d6dConfig
          .probeRaiserConstantStepSize = 10;
      // d6d_probe_raiser_type = 0 default so
      myEdgeServerQuicParams.transportSettings.d6dConfig.raiserType =
          quic::ProbeSizeRaiserType::ConstantStep;
      myEdgeServerQuicParams.transportSettings.d6dConfig
          .blackholeDetectionWindow = std::chrono::seconds(5);
      myEdgeServerQuicParams.transportSettings.d6dConfig
          .blackholeDetectionThreshold                           = 5;
      myEdgeServerQuicParams.transportSettings.d6dConfig.enabled = false;
      myEdgeServerQuicParams.transportSettings.d6dConfig.advertisedBasePMTU =
          1252;
      myEdgeServerQuicParams.transportSettings.d6dConfig
          .advertisedRaiseTimeout = std::chrono::seconds(600);
      myEdgeServerQuicParams.transportSettings.d6dConfig
          .advertisedProbeTimeout = std::chrono::seconds(600);
      myEdgeServerQuicParams.transportSettings.maxRecvBatchSize = 32;
      myEdgeServerQuicParams.transportSettings.shouldUseRecvmmsgForBatchRecv =
          true;
      myEdgeServerQuicParams.transportSettings.advertisedInitialMaxStreamsBidi =
          100;
      myEdgeServerQuicParams.transportSettings.advertisedInitialMaxStreamsUni =
          100;
      myEdgeServerQuicParams.transportSettings.tokenlessPacer = true;

      // *** HTTP Settings ***
      myEdgeServerQuicParams.h2port         = 6667; // "HTTP/2 server port"
      myEdgeServerQuicParams.localH2Address = folly::SocketAddress(
          myEdgeServerQuicParams.host, myEdgeServerQuicParams.h2port, true);
      myEdgeServerQuicParams.httpServerThreads =
          std::thread::hardware_concurrency();
      myEdgeServerQuicParams.httpServerIdleTimeout =
          std::chrono::milliseconds(60000);
      myEdgeServerQuicParams.httpServerShutdownOn = {SIGINT, SIGTERM};
      myEdgeServerQuicParams.httpServerEnableContentCompression = false;
      myEdgeServerQuicParams.h2cEnabled                         = false;
      // myEdgeServerQuicParams.httpVersion.parse("1.1");
      myEdgeServerQuicParams.txnTimeout = std::chrono::milliseconds(120000);
      folly::split(',', "", myEdgeServerQuicParams.httpPaths);
      myEdgeServerQuicParams.httpBody = "";
      myEdgeServerQuicParams.httpMethod =
          myEdgeServerQuicParams.httpBody.empty() ? proxygen::HTTPMethod::GET :
                                                    proxygen::HTTPMethod::POST;
      // parse HTTP headers
      myEdgeServerQuicParams.httpHeadersString = "";
      myEdgeServerQuicParams.httpHeaders =
          CurlService::CurlClient::parseHeaders(
              myEdgeServerQuicParams.httpHeadersString);
      // Set the host header
      if (!myEdgeServerQuicParams.httpHeaders.exists(
              proxygen::HTTP_HEADER_HOST)) {
        myEdgeServerQuicParams.httpHeaders.set(proxygen::HTTP_HEADER_HOST,
                                               myEdgeServerQuicParams.host);
      }
      myEdgeServerQuicParams.migrateClient = false;

      // *** Partial Reliability Settings ***
      myEdgeServerQuicParams.partialReliabilityEnabled = false;
      myEdgeServerQuicParams.prChunkSize = folly::to<uint64_t>(16);
      // TODO: use chrono instead of uint64_t
      myEdgeServerQuicParams.prChunkDelayMs = folly::to<uint64_t>(0);

      // *** QLogSettings ***
      myEdgeServerQuicParams.qLoggerPath = "";
      myEdgeServerQuicParams.prettyJson  = true;

      // *** StaticSettings***
      myEdgeServerQuicParams.staticRoot = "";

      // *** FizzSettings***
      myEdgeServerQuicParams.earlyData = false; // controlla il significato
      myEdgeServerQuicParams.certificateFilePath = "";
      myEdgeServerQuicParams.keyFilePath         = "";
      myEdgeServerQuicParams.pskFilePath         = "";
      // psk_file default empty, PersistentQuicPskCache not initialized
      // we use the else branch

      myEdgeServerQuicParams.pskCache =
          std::make_shared<proxygen::SynchronizedLruQuicPskCache>(1000);

      // client_auth_mode = "" default, so noone of the branches is called
      myEdgeServerQuicParams.clientAuth =
          fizz::server::ClientAuthMode::None; // verifica, in base al valore del
                                              // flag non doveva essere
                                              // richiamato
    }

    ~System() {
      LOG(INFO) << "Terminating Test\n";
    }

  }; // struct System
};

TEST_F(TestEdgeServerQuic, test_ctor) {

  folly::ssl::init();
  quic::samples::HQParams myEdgeServerQuicParams;
  System                  mySystem(myEdgeServerQuicParams);

  std::unique_ptr<EdgeRouter> theRouter;
  theRouter.reset(
      new EdgeRouter(myEdgeServerQuicParams.host + ':' +
                         std::to_string(myEdgeServerQuicParams.port),
                     myEdgeServerQuicParams.host + ':' + std::to_string(6474),
                     "",
                     support::Conf(EdgeLambdaProcessor::defaultConf()),
                     support::Conf("type=random"),
                     support::Conf("type=trivial,period=10,stat=mean")));

  std::unique_ptr<EdgeServerImpl> myServerImpl;
  myServerImpl.reset(new EdgeServerQuic(*theRouter, myEdgeServerQuicParams));

  /*
    assert(myServerImpl != nullptr);
    myServerImpl->run();
    myServerImpl->wait();
  */

} // namespace edge

} // namespace edge
} // namespace uiiit