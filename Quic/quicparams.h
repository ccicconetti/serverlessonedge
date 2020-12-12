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

#include <string>

#include "Support/conf.h"

#include <folly/Optional.h>
#include <folly/SocketAddress.h>
#include <proxygen/httpclient/samples/curl/CurlClient.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <proxygen/lib/http/HTTPHeaders.h>
#include <proxygen/lib/http/HTTPMethod.h>
#include <proxygen/lib/http/SynchronizedLruQuicPskCache.h>
#include <proxygen/lib/http/session/HQSession.h>
#include <quic/QuicConstants.h>
#include <quic/fizz/client/handshake/QuicPskCache.h>
#include <quic/server/QuicServerTransport.h>

namespace uiiit {
namespace edge {

enum class HQMode { INVALID, CLIENT, SERVER };

struct HTTPVersion {
  std::string version;
  std::string canonical;
  uint16_t    major{1};
  uint16_t    minor{1};
  bool        parse(const std::string&);
};

/**
 * Struct to hold both HTTP/3 and HTTP/2 settings for HQ
 */
struct HQParams {
  // General section
  HQMode      mode;
  std::string logprefix;
  std::string logdir;
  std::string outdir;
  bool        logResponse;

  // Transport section
  std::string                                  host;
  uint16_t                                     port;
  std::string                                  protocol;
  folly::Optional<folly::SocketAddress>        localAddress;
  folly::Optional<folly::SocketAddress>        remoteAddress;
  std::string                                  transportVersion;
  std::vector<quic::QuicVersion>               quicVersions;
  std::vector<std::string>                     supportedAlpns;
  std::string                                  localHostname;
  std::string                                  httpProtocol;
  quic::TransportSettings                      transportSettings;
  std::string                                  congestionControlName;
  folly::Optional<quic::CongestionControlType> congestionControl;
  bool                                         earlyData;
  folly::Optional<int64_t>                     rateLimitPerThread;
  std::chrono::milliseconds                    connectTimeout;
  std::string                                  ccpConfig;
  bool                                         sendKnobFrame;

  // HTTP section
  uint16_t                              h2port;
  folly::Optional<folly::SocketAddress> localH2Address;
  HTTPVersion                           httpVersion;
  std::string                           httpHeadersString;
  proxygen::HTTPHeaders                 httpHeaders;
  std::string                           httpBody;
  proxygen::HTTPMethod                  httpMethod;
  std::vector<folly::StringPiece>       httpPaths;

  std::chrono::milliseconds txnTimeout;

  size_t                    httpServerThreads;
  std::chrono::milliseconds httpServerIdleTimeout;
  std::vector<int>          httpServerShutdownOn;
  bool                      httpServerEnableContentCompression;
  bool                      h2cEnabled;

  // Partial reliability section
  bool                      partialReliabilityEnabled{false};
  folly::Optional<uint64_t> prChunkSize;
  folly::Optional<uint64_t> prChunkDelayMs;

  // QLogger section
  std::string qLoggerPath;
  bool        prettyJson;

  // Static options
  std::string staticRoot;

  // Fizz options
  std::string                         certificateFilePath;
  std::string                         keyFilePath;
  std::string                         pskFilePath;
  std::shared_ptr<quic::QuicPskCache> pskCache;
  fizz::server::ClientAuthMode clientAuth{fizz::server::ClientAuthMode::None};

  // Transport knobs
  std::string transportKnobs;

  bool migrateClient{false};

  // Struct Constructor (set default values according to the boolean parameter)
  HQParams(bool isServer) {
    // *** Common Settings Section ***
    host = "127.0.0.1"; // ::1
    port = 6473;        // HQServer Port
    if (isServer) {
      mode         = HQMode::SERVER;
      logprefix    = "server";
      localAddress = folly::SocketAddress(host, port, true);
    } else {
      mode          = HQMode::CLIENT;
      logprefix     = "client";
      remoteAddress = folly::SocketAddress(host, port, true);
      // local_address empty by default, local_address not set
    }
    logdir      = "/tmp/logs";
    logResponse = true;
    outdir      = "";

    // *** TransportSettings ***
    quicVersions = {quic::QuicVersion::MVFST,
                    quic::QuicVersion::MVFST_D24,
                    quic::QuicVersion::MVFST_EXPERIMENTAL,
                    quic::QuicVersion::QUIC_DRAFT,
                    quic::QuicVersion::QUIC_DRAFT_LEGACY};
    // draft_version = 0 by default -> no if branch
    // protocol = "" by default -> else branch
    supportedAlpns                                          = {"h1q-fb",
                      "h1q-fb-v2",
                      proxygen::kH3FBCurrentDraft,
                      proxygen::kH3CurrentDraft,
                      proxygen::kH3LegacyDraft,
                      proxygen::kHQCurrentDraft};
    transportSettings.advertisedInitialConnectionWindowSize = 1024 * 1024 * 10;
    transportSettings.advertisedInitialBidiLocalStreamWindowSize  = 256 * 1024;
    transportSettings.advertisedInitialBidiRemoteStreamWindowSize = 256 * 1024;
    transportSettings.advertisedInitialUniStreamWindowSize        = 256 * 1024;
    congestionControlName                                         = "cubic";
    congestionControl = quic::congestionControlStrToType("cubic");
    // since congestion control cannot be null since we are using default
    // values
    if (congestionControl) {
      transportSettings.defaultCongestionController = congestionControl.value();
    }
    transportSettings.maxRecvPacketSize = quic::kDefaultUDPReadBufferSize;
    transportSettings.numGROBuffers_    = quic::kDefaultNumGROBuffers;
    transportSettings.pacingEnabled     = false;
    // pacingEnabled false by default, no if branch
    transportSettings.batchingMode = quic::getQuicBatchingMode(
        static_cast<uint32_t>(quic::QuicBatchingMode::BATCHING_MODE_NONE));
    transportSettings.useThreadLocalBatching = false;
    transportSettings.threadLocalDelay       = std::chrono::microseconds(1000);
    transportSettings.maxBatchSize           = quic::kDefaultQuicMaxBatchSize;
    transportSettings.turnoffPMTUD           = true;
    transportSettings.partialReliabilityEnabled = false;
    if (!isServer) {
      transportSettings.shouldDrain = false;
      transportSettings.attemptEarlyData =
          false; // CHECK!!! (WHETHER TO USE 0-RTT)
    }
    transportSettings.connectUDP       = false;
    transportSettings.maxCwndInMss     = quic::kLargeMaxCwndInMss;
    transportSettings.disableMigration = false;
    // FLAGS_use_inplace_write = false by default, no if branch
    // FLAGS_rate_limit = -1 by default, no if branch
    connectTimeout = std::chrono::milliseconds(2000);
    ccpConfig      = "";
    sendKnobFrame  = false;
    // sendKnobFrame = false by default, no if branch
    transportSettings.d6dConfig.enabled                     = false;
    transportSettings.d6dConfig.probeRaiserConstantStepSize = 10;
    // d6d_probe_raiser_type = 0 default so we can use the following
    transportSettings.d6dConfig.raiserType =
        quic::ProbeSizeRaiserType::ConstantStep;
    transportSettings.d6dConfig.blackholeDetectionWindow =
        std::chrono::seconds(5);
    transportSettings.d6dConfig.blackholeDetectionThreshold = 5;
    transportSettings.d6dConfig.enabled                     = false;
    transportSettings.d6dConfig.advertisedBasePMTU          = 1252;
    transportSettings.d6dConfig.advertisedRaiseTimeout =
        std::chrono::seconds(600);
    transportSettings.d6dConfig.advertisedProbeTimeout =
        std::chrono::seconds(600);
    transportSettings.maxRecvBatchSize                = 32;
    transportSettings.shouldUseRecvmmsgForBatchRecv   = true;
    transportSettings.advertisedInitialMaxStreamsBidi = 100;
    transportSettings.advertisedInitialMaxStreamsUni  = 100;
    transportSettings.tokenlessPacer                  = true;

    // *** HTTP Settings ***
    h2port         = 6667; // "HTTP/2 server port"
    localH2Address = folly::SocketAddress(host, h2port, true);
    // std::thread::hardware_concurrency() << can be quite a lot...
    httpServerThreads                  = 5;
    httpServerIdleTimeout              = std::chrono::milliseconds(60000);
    httpServerShutdownOn               = {SIGINT, SIGTERM};
    httpServerEnableContentCompression = false;
    h2cEnabled                         = false;
    // httpVersion.parse("1.1");
    txnTimeout = std::chrono::milliseconds(120000);
    folly::split(',', "/lambda", httpPaths);
    httpBody   = "";
    httpMethod = httpBody.empty() ? proxygen::HTTPMethod::GET :
                                    proxygen::HTTPMethod::POST;
    // parse HTTP headers
    httpHeadersString = "";
    httpHeaders = CurlService::CurlClient::parseHeaders(httpHeadersString);
    // Set the host header
    if (!httpHeaders.exists(proxygen::HTTP_HEADER_HOST)) {
      httpHeaders.set(proxygen::HTTP_HEADER_HOST, host);
    }
    migrateClient = false;

    // *** Partial Reliability Settings ***
    partialReliabilityEnabled = false;
    prChunkSize               = folly::to<uint64_t>(16);
    // TODO: use chrono instead of uint64_t
    prChunkDelayMs = folly::to<uint64_t>(0);

    // *** QLogSettings ***
    qLoggerPath = "";
    prettyJson  = true;

    // *** StaticSettings***
    staticRoot = "";

    // *** FizzSettings***
    earlyData           = false; // CHECK the function of this
    certificateFilePath = "";
    keyFilePath         = "";
    pskFilePath         = "";
    // psk_file is empty by default, else branch
    pskCache = std::make_shared<proxygen::SynchronizedLruQuicPskCache>(1000);
    // client_auth_mode = "" by default, so none of the branches is called
    clientAuth = fizz::server::ClientAuthMode::None; // CHECK!!!
  }
};

/**
 * Basically a wrapper for HQParams, provides ctor methods for both
 * EdgeServerQuic and EdgeClientQuic
 */
/**
class QuicParams
{
public:

 * Ctor for EdgeClientQuic & for EdgeServerQuic if server-conf is empty
 * TODO: check if it is needed to use boost options from CLI to initialize the
 * client Params or if it is sufficient to initialize with default values
 * (not important now)

QuicParams(bool isServer);
QuicParams(uiiit::support::Conf quicConf); // for EdgeServerQuic

~QuicParams();

const HQParams theQuicParams;
};
*/
} // namespace edge
} // namespace uiiit