#include "Edge/edgelambdaprocessoroptions.h"
#include "Edge/edgerouter.h"
#include "Edge/edgeserverimpl.h"

#include "gtest/gtest.h"
#include <glog/logging.h>

#include "Support/conf.h"

#include <proxygen/httpserver/samples/hq/HQParams.h>

#include <quic/QuicConstants.h>

namespace uiiit {
namespace edge {

struct TestEdgeServerQuic : public ::testing::Test {};

TEST_F(TestEdgeServerQuic, test_ctor) {

  EdgeRouter myRouter("127.0.0.1:6473",
                      "127.0.0.1:6474",
                      "127.0.0.1:6475",
                      support::Conf(EdgeLambdaProcessor::defaultConf()),
                      support::Conf("type=random"),
                      support::Conf("type=trivial,period=10,stat=mean"));

  std::unique_ptr<EdgeServerImpl> myRouterEdgeServerImpl;

  quic::samples::HQParams myEdgeServerQuicParams;
  // General Section
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

  // TransportSettings
  myEdgeServerQuicParams.quicVersions = {quic::QuicVersion::MVFST,
                                         quic::QuicVersion::MVFST_D24,
                                         quic::QuicVersion::MVFST_EXPERIMENTAL,
                                         quic::QuicVersion::QUIC_DRAFT,
                                         quic::QuicVersion::QUIC_DRAFT_LEGACY};
  // draft_version = 0 di default
}

} // namespace edge
} // namespace uiiit