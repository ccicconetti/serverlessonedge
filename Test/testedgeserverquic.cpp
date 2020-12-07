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
           qs::HQParams& myEdgeClientQuicParams)
        : theRouter(new EdgeRouter(
              "127.0.0.1:6473",
              "127.0.0.1:6474",
              "",
              support::Conf(EdgeLambdaProcessor::defaultConf()),
              support::Conf("type=random"),
              support::Conf("type=trivial,period=10,stat=mean"))) {
      initializeEdgeQuicParams(myEdgeServerQuicParams, true);
      initializeEdgeQuicParams(myEdgeClientQuicParams, false);

      std::unique_ptr<EdgeServerImpl> myServerImpl;
      myServerImpl.reset(
          new EdgeServerQuic(*theRouter, myEdgeServerQuicParams));
      assert(myServerImpl != nullptr);
      myServerImpl->run();

      LOG(INFO) << "Starting Test\n";
    }

    ~System() {
      LOG(INFO) << "Terminating Test\n";
    }

    void initializeEdgeQuicParams(qs::HQParams& myQuicParams, bool isServer) {
      // *** General Section ***
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
        // local_address vuoto di default quindi non imposto local address
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
      // draft_version = 0 di default
      // protocol = "" di default -> entro nell'else
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
            false; // CONTROLLA (WHETHER TO USE 0-RTT)
      }
      myQuicParams.transportSettings.connectUDP   = false;
      myQuicParams.transportSettings.maxCwndInMss = quic::kLargeMaxCwndInMss;
      myQuicParams.transportSettings.disableMigration = false;
      /*
       FLAGS_use_inplace_write = false default, so we do not enter in the if
       branch and we do not set
       myQuicParams.transportSettings.dataPathType
      */
      // FLAGS_rate_limit = -1 default so we do not set
      // myQuicParams.rateLimitPerThread
      myQuicParams.connectTimeout = std::chrono::milliseconds(2000);
      myQuicParams.ccpConfig      = "";
      myQuicParams.sendKnobFrame  = false;
      // sendKnobFrame default is false, so we do not enter in if branch
      myQuicParams.transportSettings.d6dConfig.enabled = false;
      myQuicParams.transportSettings.d6dConfig.probeRaiserConstantStepSize = 10;
      // d6d_probe_raiser_type = 0 default so
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
      myQuicParams.httpServerThreads     = std::thread::hardware_concurrency();
      myQuicParams.httpServerIdleTimeout = std::chrono::milliseconds(60000);
      myQuicParams.httpServerShutdownOn  = {SIGINT, SIGTERM};
      myQuicParams.httpServerEnableContentCompression = false;
      myQuicParams.h2cEnabled                         = false;
      // myQuicParams.httpVersion.parse("1.1");
      myQuicParams.txnTimeout = std::chrono::milliseconds(120000);
      folly::split(',', "", myQuicParams.httpPaths);
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
      myQuicParams.earlyData           = false; // controlla il significato
      myQuicParams.certificateFilePath = "";
      myQuicParams.keyFilePath         = "";
      myQuicParams.pskFilePath         = "";
      // psk_file default empty, PersistentQuicPskCache not initialized
      // we use the else branch

      myQuicParams.pskCache =
          std::make_shared<proxygen::SynchronizedLruQuicPskCache>(1000);

      // client_auth_mode = "" default, so noone of the branches is called
      myQuicParams.clientAuth =
          fizz::server::ClientAuthMode::None; // verifica, in base al valore del
                                              // flag non doveva essere
                                              // richiamato
    }

    std::unique_ptr<EdgeRouter> theRouter;
  }; // struct System
};

TEST_F(TestEdgeServerQuic, test_ctor) {

  folly::ssl::init();
  qs::HQParams myEdgeServerQuicParams; // HQParams for edgeserverquic
  qs::HQParams myEdgeClientQuicParams; // HQParams for edgeclientquic

  LOG(INFO) << "PRE SYSTEM CTOR \n";
  System mySystem(myEdgeServerQuicParams, myEdgeClientQuicParams);
  LOG(INFO) << "POST SYSTEM CTOR \n";

  LOG(INFO) << "PRE EDGECLIENTQUIC\n";
  EdgeClientQuic myClient(myEdgeClientQuicParams);
  LOG(INFO) << "POST EDGECLIENTQUIC\n";

  /*
    myServerImpl->wait();
  */

} // namespace edge

} // namespace edge
} // namespace uiiit