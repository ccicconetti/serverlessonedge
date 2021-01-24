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
#include "Support/split.h"

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

struct HTTPVersion {
  std::string version;
  std::string canonical;
  uint16_t    major{1};
  uint16_t    minor{1};

  bool parse(const std::string& verString) {
    // version, major and minor are fields of struct HTTPVersion
    version = verString;
    if (version.length() == 1) {
      major     = folly::to<uint16_t>(version);
      minor     = 0;
      canonical = folly::to<std::string>(major, ".", minor);
      return true;
    }
    std::string delimiter = ".";
    std::size_t pos       = version.find(delimiter);
    if (pos == std::string::npos) {
      LOG(ERROR) << "Invalid http-version string: " << version
                 << ", defaulting to HTTP/1.1";
      major     = 1;
      minor     = 1;
      canonical = folly::to<std::string>(major, ".", minor);
      return false;
    }

    try {
      std::string majorVer = version.substr(0, pos);
      std::string minorVer = version.substr(pos + delimiter.length());
      major                = folly::to<uint16_t>(majorVer);
      minor                = folly::to<uint16_t>(minorVer);
      canonical            = folly::to<std::string>(major, ".", minor);
      return true;
    } catch (const folly::ConversionError&) {
      LOG(ERROR) << "Invalid http-version string: " << version
                 << ", defaulting to HTTP/1.1";
      major     = 1;
      minor     = 1;
      canonical = folly::to<std::string>(major, ".", minor);
      return false;
    }
  }
};

/**
 * Struct to hold both HTTP/3 and HTTP/2 settings for EdgeQuicServer and
 * EdgeQuicClient
 */
struct HQParams {

  // Transport section
  std::string                                  host;
  uint16_t                                     port;
  folly::Optional<folly::SocketAddress>        localAddress;
  folly::Optional<folly::SocketAddress>        remoteAddress;
  std::vector<quic::QuicVersion>               quicVersions;
  std::vector<std::string>                     supportedAlpns;
  quic::TransportSettings                      transportSettings;
  std::string                                  congestionControlName;
  folly::Optional<quic::CongestionControlType> congestionControl;
  bool                                         earlyData;
  folly::Optional<int64_t>                     rateLimitPerThread;
  std::chrono::milliseconds                    connectTimeout;
  std::string                                  ccpConfig;

  // HTTP section
  HTTPVersion                     httpVersion;
  proxygen::HTTPHeaders           httpHeaders;
  std::vector<folly::StringPiece> httpPaths;
  size_t                          httpServerThreads;
  std::chrono::milliseconds       txnTimeout;

  // Fizz options
  std::shared_ptr<quic::QuicPskCache> pskCache;
  fizz::server::ClientAuthMode clientAuth{fizz::server::ClientAuthMode::None};

  HQParams() {
    // *** TransportSettings ***
    quicVersions   = {quic::QuicVersion::MVFST,
                    quic::QuicVersion::MVFST_D24,
                    quic::QuicVersion::MVFST_EXPERIMENTAL,
                    quic::QuicVersion::QUIC_DRAFT,
                    quic::QuicVersion::QUIC_DRAFT_LEGACY};
    supportedAlpns = {"h1q-fb",
                      "h1q-fb-v2",
                      proxygen::kH3FBCurrentDraft,
                      proxygen::kH3CurrentDraft,
                      proxygen::kH3LegacyDraft,
                      proxygen::kHQCurrentDraft};

    transportSettings.advertisedInitialConnectionWindowSize =
        1024 * 1024 * 10; //"Connection flow control"
    transportSettings.advertisedInitialBidiLocalStreamWindowSize =
        256 * 1024; //"Stream flow control"
    transportSettings.advertisedInitialBidiRemoteStreamWindowSize =
        256 * 1024; //"Stream flow control"
    transportSettings.advertisedInitialUniStreamWindowSize =
        256 * 1024;                  //"Stream flow control"
    congestionControlName = "cubic"; //"newreno / cubic / bbr / none"
    congestionControl = quic::congestionControlStrToType(congestionControlName);
    if (congestionControl) {
      transportSettings.defaultCongestionController = congestionControl.value();
    }
    transportSettings.maxRecvPacketSize =
        quic::kDefaultUDPReadBufferSize; //"Max UDP packet size Quic can
                                         // receive"
    transportSettings.numGROBuffers_ =
        quic::kDefaultNumGROBuffers; //"Number of GRO buffers"

    transportSettings.batchingMode =
        quic::getQuicBatchingMode(static_cast<uint32_t>(
            quic::QuicBatchingMode::BATCHING_MODE_NONE)); //"QUIC batching mode"
    transportSettings.useThreadLocalBatching =
        false; //"Use thread local batching"
    transportSettings.threadLocalDelay =
        std::chrono::microseconds(1000); //"Thread local delay in microseconds"
    transportSettings.maxBatchSize =
        quic::kDefaultQuicMaxBatchSize; //"Maximum number of packets that can be
                                        // batched in Quic"
    transportSettings.turnoffPMTUD              = true;
    transportSettings.partialReliabilityEnabled = false;
    transportSettings.connectUDP =
        false; //"Whether or not to use connected udp sockets"
    transportSettings.maxCwndInMss =
        quic::kLargeMaxCwndInMss; //"Max cwnd in unit of mss"
    transportSettings.disableMigration = false;
    connectTimeout =
        std::chrono::milliseconds(2000); //"(HQClient) connect timeout in ms"
    ccpConfig = ""; //"Additional args to pass to ccp. Ccp disabled if empty
    // string."
    transportSettings.d6dConfig.enabled = false;
    transportSettings.d6dConfig.probeRaiserConstantStepSize =
        10; //"Server only. The constant step size used to increase PMTU, only "
            //"meaningful to ConstantStep probe size raiser"
    transportSettings.d6dConfig.raiserType = quic::ProbeSizeRaiserType::
        ConstantStep; //"Server only. The type of probe size raiser.
                      // (0:ConstantStep, 1:BinarySearch)
    transportSettings.d6dConfig.blackholeDetectionWindow = std::chrono::seconds(
        5); //"Server only. PMTU blackhole detection window in secs"
    transportSettings.d6dConfig.blackholeDetectionThreshold =
        5; //"Server only. PMTU blackhole detection threshold, in # of packets"
    transportSettings.d6dConfig.advertisedBasePMTU =
        1252; //"Client only. The base PMTU advertised to server"
    transportSettings.d6dConfig.advertisedRaiseTimeout = std::chrono::seconds(
        600); //"Client only. The raise timeout advertised to server"
    transportSettings.d6dConfig.advertisedProbeTimeout = std::chrono::seconds(
        600); //"Client only. The probe timeout advertised to server"
    transportSettings.maxRecvBatchSize                = 32;
    transportSettings.shouldUseRecvmmsgForBatchRecv   = true;
    transportSettings.advertisedInitialMaxStreamsBidi = 100;
    transportSettings.advertisedInitialMaxStreamsUni  = 100;
    transportSettings.tokenlessPacer                  = true;

    // *** HTTP Settings ***
    txnTimeout = std::chrono::milliseconds(120000);
    httpVersion.parse("1.1");
    folly::split(',', "/lambda", httpPaths);
    // parse HTTP headers
    httpHeaders = CurlService::CurlClient::parseHeaders("");
    // Set the host header
    if (!httpHeaders.exists(proxygen::HTTP_HEADER_HOST)) {
      httpHeaders.set(proxygen::HTTP_HEADER_HOST, host);
    }

    // *** FizzSettings***
    pskCache = std::make_shared<proxygen::SynchronizedLruQuicPskCache>(1000);
  }
};

/**
 * Class to implement the HQParams building
 */
class QuicParamsBuilder
{
 public:
  /**
   * \param aClientConf client configuration specified through the --client-conf
   * option from CLI. This configuration can be used to specify: \li type
   * (=grpc): the protocol to exchange lambdaRequest/lambdaResponse (can be
   * "grpc" or "quic"), \li persistence (=0.5): the p-persistence probability to
   * probe another destination, \li attempt-early-data (=false): flag to make
   * the EdgeClientQuic trying to exploit the QUIC protocol 0-RTT feature
   *
   * \param aServerEndpoint server endpoint to which the client must
   * connect, specified through the --server-endpoint option from CLI (format:
   * "IPAddress:Port")
   *
   * \returns the HQParams configuration (both QUIC and HTTP parameters) to
   * build an EdgeClientQuic starting from the given CLI options
   */
  static HQParams buildClientHQParams(const support::Conf& aClientConf,
                                      const std::string&   aServerEndpoint);

  /**
   * \param aServerConf server configuration specified through the --server-conf
   * option from CLI. For the server this configuration can be used to specify:
   * \li type(=grpc): the protocol to exchange lambdaRequest/lambdaResponse (can
   * be "grpc" or "quic")
   *
   * \param aServerEndpoint server endpoint specified through the
   * --server-endpoint option from CLI (format: "IPAddress:Port")
   *
   * \param aNumThreads number of threads to be spawned in the HQServer
   *
   * \returns the HQParams configuration (both QUIC and HTTP parameters) to
   * build an EdgeServerQuic starting from the given CLI options
   */
  static HQParams buildServerHQParams(const support::Conf& aServerConf,
                                      const std::string&   aServerEndpoint,
                                      const size_t         aNumThreads);
};

} // namespace edge
} // namespace uiiit