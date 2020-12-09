#include "Edge/edgeclientquic.h"
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

namespace qs = quic::samples;

struct TestEdgeServerQuic : public ::testing::Test {

  struct System {

    System(qs::HQParams& myEdgeServerQuicParams,
           qs::HQParams& myEdgeClientQuicParams) {
      initializeEdgeQuicParams(myEdgeServerQuicParams, true);
      initializeEdgeQuicParams(myEdgeClientQuicParams, false);

      LOG(INFO) << "Starting Test\n";
    }

    ~System() {
      LOG(INFO) << "Terminating Test\n";
    }

    void initializeEdgeQuicParams(qs::HQParams& myQuicParams, bool isServer) {

      // *** Common Settings Section ***
      myQuicParams.host = "127.0.0.1";
      myQuicParams.port = 6473;
      if (isServer) {
        myQuicParams.mode      = qs::HQMode::SERVER;
        myQuicParams.logprefix = "server";
        myQuicParams.localAddress =
            folly::SocketAddress(myQuicParams.host, myQuicParams.port, true);
      } else {
        myQuicParams.mode      = qs::HQMode::CLIENT;
        myQuicParams.logprefix = "client";
        myQuicParams.remoteAddress =
            folly::SocketAddress(myQuicParams.host, myQuicParams.port, true);
        // local_address empty by default, local_address not set
      }
      myQuicParams.logdir      = "/tmp/logs";
      myQuicParams.logResponse = true;
      myQuicParams.outdir      = "";

      // *** TransportSettings ***
      myQuicParams.quicVersions = {quic::QuicVersion::MVFST,
                                   quic::QuicVersion::MVFST_D24,
                                   quic::QuicVersion::MVFST_EXPERIMENTAL,
                                   quic::QuicVersion::QUIC_DRAFT,
                                   quic::QuicVersion::QUIC_DRAFT_LEGACY};
      // draft_version = 0 by default -> no if branch
      // protocol = "" by default -> else branch
      myQuicParams.supportedAlpns = {"h1q-fb",
                                     "h1q-fb-v2",
                                     proxygen::kH3FBCurrentDraft,
                                     proxygen::kH3CurrentDraft,
                                     proxygen::kH3LegacyDraft,
                                     proxygen::kHQCurrentDraft};
      myQuicParams.transportSettings.advertisedInitialConnectionWindowSize =
          1024 * 1024 * 10;
      myQuicParams.transportSettings
          .advertisedInitialBidiLocalStreamWindowSize = 256 * 1024;
      myQuicParams.transportSettings
          .advertisedInitialBidiRemoteStreamWindowSize = 256 * 1024;
      myQuicParams.transportSettings.advertisedInitialUniStreamWindowSize =
          256 * 1024;
      myQuicParams.congestionControlName = "cubic";
      myQuicParams.congestionControl =
          quic::congestionControlStrToType("cubic");
      // since congestion control cannot be null since we are using default
      // values
      myQuicParams.transportSettings.defaultCongestionController =
          quic::CongestionControlType::Cubic;

      myQuicParams.transportSettings.maxRecvPacketSize =
          quic::kDefaultUDPReadBufferSize;
      myQuicParams.transportSettings.numGROBuffers_ =
          quic::kDefaultNumGROBuffers;
      myQuicParams.transportSettings.pacingEnabled = false;
      // pacingEnabled false di default, non entro nell'if e non setto
      // myQuicParams.transportSettings.pacingTimerTickInterval
      myQuicParams.transportSettings.batchingMode = quic::getQuicBatchingMode(
          static_cast<uint32_t>(quic::QuicBatchingMode::BATCHING_MODE_NONE));
      myQuicParams.transportSettings.useThreadLocalBatching = false;
      myQuicParams.transportSettings.threadLocalDelay =
          std::chrono::microseconds(1000);
      myQuicParams.transportSettings.maxBatchSize =
          quic::kDefaultQuicMaxBatchSize;
      myQuicParams.transportSettings.turnoffPMTUD              = true;
      myQuicParams.transportSettings.partialReliabilityEnabled = false;
      if (!isServer) {
        myQuicParams.transportSettings.shouldDrain = false;
        myQuicParams.transportSettings.attemptEarlyData =
            false; // CHECK!!! (WHETHER TO USE 0-RTT)
      }
      myQuicParams.transportSettings.connectUDP   = false;
      myQuicParams.transportSettings.maxCwndInMss = quic::kLargeMaxCwndInMss;
      myQuicParams.transportSettings.disableMigration = false;
      // FLAGS_use_inplace_write = false by default, no if branch
      // FLAGS_rate_limit = -1 by default, no if branch
      myQuicParams.connectTimeout = std::chrono::milliseconds(2000);
      myQuicParams.ccpConfig      = "";
      myQuicParams.sendKnobFrame  = false;
      // sendKnobFrame = false by default, no if branch
      myQuicParams.transportSettings.d6dConfig.enabled = false;
      myQuicParams.transportSettings.d6dConfig.probeRaiserConstantStepSize = 10;
      // d6d_probe_raiser_type = 0 default so we can use the following
      myQuicParams.transportSettings.d6dConfig.raiserType =
          quic::ProbeSizeRaiserType::ConstantStep;
      myQuicParams.transportSettings.d6dConfig.blackholeDetectionWindow =
          std::chrono::seconds(5);
      myQuicParams.transportSettings.d6dConfig.blackholeDetectionThreshold = 5;
      myQuicParams.transportSettings.d6dConfig.enabled            = false;
      myQuicParams.transportSettings.d6dConfig.advertisedBasePMTU = 1252;
      myQuicParams.transportSettings.d6dConfig.advertisedRaiseTimeout =
          std::chrono::seconds(600);
      myQuicParams.transportSettings.d6dConfig.advertisedProbeTimeout =
          std::chrono::seconds(600);
      myQuicParams.transportSettings.maxRecvBatchSize                = 32;
      myQuicParams.transportSettings.shouldUseRecvmmsgForBatchRecv   = true;
      myQuicParams.transportSettings.advertisedInitialMaxStreamsBidi = 100;
      myQuicParams.transportSettings.advertisedInitialMaxStreamsUni  = 100;
      myQuicParams.transportSettings.tokenlessPacer                  = true;

      // *** HTTP Settings ***
      myQuicParams.h2port = 6667; // "HTTP/2 server port"
      myQuicParams.localH2Address =
          folly::SocketAddress(myQuicParams.host, myQuicParams.h2port, true);
      // std::thread::hardware_concurrency() << can be quite a lot...
      myQuicParams.httpServerThreads     = 5;
      myQuicParams.httpServerIdleTimeout = std::chrono::milliseconds(60000);
      myQuicParams.httpServerShutdownOn  = {SIGINT, SIGTERM};
      myQuicParams.httpServerEnableContentCompression = false;
      myQuicParams.h2cEnabled                         = false;
      // myQuicParams.httpVersion.parse("1.1");
      myQuicParams.txnTimeout = std::chrono::milliseconds(120000);
      folly::split(',', "/", myQuicParams.httpPaths);
      myQuicParams.httpBody   = "";
      myQuicParams.httpMethod = myQuicParams.httpBody.empty() ?
                                    proxygen::HTTPMethod::GET :
                                    proxygen::HTTPMethod::POST;
      // parse HTTP headers
      myQuicParams.httpHeadersString = "";
      myQuicParams.httpHeaders =
          CurlService::CurlClient::parseHeaders(myQuicParams.httpHeadersString);
      // Set the host header
      if (!myQuicParams.httpHeaders.exists(proxygen::HTTP_HEADER_HOST)) {
        myQuicParams.httpHeaders.set(proxygen::HTTP_HEADER_HOST,
                                     myQuicParams.host);
      }
      myQuicParams.migrateClient = false;

      // *** Partial Reliability Settings ***
      myQuicParams.partialReliabilityEnabled = false;
      myQuicParams.prChunkSize               = folly::to<uint64_t>(16);
      // TODO: use chrono instead of uint64_t
      myQuicParams.prChunkDelayMs = folly::to<uint64_t>(0);

      // *** QLogSettings ***
      myQuicParams.qLoggerPath = "";
      myQuicParams.prettyJson  = true;

      // *** StaticSettings***
      myQuicParams.staticRoot = "";

      // *** FizzSettings***
      myQuicParams.earlyData           = false; // CHECK the function of this
      myQuicParams.certificateFilePath = "";
      myQuicParams.keyFilePath         = "";
      myQuicParams.pskFilePath         = "";
      // psk_file is empty by default, else branch
      myQuicParams.pskCache =
          std::make_shared<proxygen::SynchronizedLruQuicPskCache>(1000);
      // client_auth_mode = "" by default, so none of the branches is called
      myQuicParams.clientAuth = fizz::server::ClientAuthMode::None; // CHECK!!!
    }

  }; // struct System
};

TEST_F(TestEdgeServerQuic, test_ctor) {

  folly::ssl::init();
  qs::HQParams myEdgeServerQuicParams; // HQParams for edgeserverquic
  qs::HQParams myEdgeClientQuicParams; // HQParams for edgeclientquic

  System mySystem(myEdgeServerQuicParams, myEdgeClientQuicParams);
  LOG(INFO) << "POST SYSTEM CTOR \n";

  std::unique_ptr<EdgeRouter> theRouter;
  theRouter.reset(
      new EdgeRouter(myEdgeServerQuicParams.host + ':' +
                         std::to_string(myEdgeServerQuicParams.port),
                     myEdgeServerQuicParams.host + ":6474",
                     "",
                     support::Conf(EdgeLambdaProcessor::defaultConf()),
                     support::Conf("type=random"),
                     support::Conf("type=trivial,period=10,stat=mean")));

  std::unique_ptr<EdgeServerImpl> myServerImpl;
  myServerImpl.reset(new EdgeServerQuic(*theRouter, myEdgeServerQuicParams));
  assert(myServerImpl != nullptr);
  myServerImpl->run();

  std::this_thread::sleep_for(std::chrono::seconds(1));

  EdgeClientQuic myClient(myEdgeClientQuicParams);
  myClient.startClient();

  LOG(INFO) << "Terminating test\n";
} // namespace edge

} // namespace edge
} // namespace uiiit