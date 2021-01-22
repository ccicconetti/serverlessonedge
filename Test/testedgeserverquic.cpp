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

#include "Edge/composer.h"
#include "Edge/edgecomputer.h"
#include "Edge/edgecomputerserver.h"
#include "Edge/edgelambdaprocessoroptions.h"
#include "Edge/edgeserverimpl.h"
#include "Quic/edgeclientquic.h"
#include "Quic/edgeserverquic.h"
#include "Quic/quicparamsbuilder.h"
#include "Support/conf.h"

#include "gtest/gtest.h"

#include <glog/logging.h>

#include <folly/ssl/Init.h>
#include <quic/QuicConstants.h>

namespace uiiit {
namespace edge {

struct TestEdgeServerQuic : public ::testing::Test {

  TestEdgeServerQuic()
      : theServerEndpoint("127.0.0.1:10000")
      , theUtilEndpoint("127.0.0.1:20000")
      , theComposerConf("type=raspberry,num-cpu-containers=1,num-cpu-workers=4,"
                        "num-gpu-containers=1,num-gpu-workers=2")
      , theGrpcClientConf("type=grpc,persistence=0.5")
      , theQuicClientConf("type=quic,attempt-early-data=true")
      , theQuicServerConf("type=quic, h2port=6667") {
  }

  const std::string   theServerEndpoint;
  const std::string   theUtilEndpoint;
  const support::Conf theComposerConf;
  const support::Conf theGrpcClientConf;
  const support::Conf theQuicClientConf;
  const support::Conf theQuicServerConf;
};

TEST_F(TestEdgeServerQuic, test_buildparam) {
  auto paramServer = QuicParamsBuilder::buildServerHQParams(
      theQuicServerConf, theServerEndpoint, 2);
  auto paramClient = QuicParamsBuilder::buildClientHQParams(theQuicClientConf,
                                                            theServerEndpoint);
}

TEST_F(TestEdgeServerQuic, test_connection) {

  Computer::UtilCallback              myUtilCallback;
  std::unique_ptr<EdgeComputerServer> myUtilServer;

  myUtilServer.reset(new EdgeComputerServer(theUtilEndpoint));
  myUtilCallback = [&myUtilServer](const std::map<std::string, double>& aUtil) {
    myUtilServer->add(aUtil);
  };
  myUtilServer->run(false); // non-blocking

  EdgeComputer myComputer(theServerEndpoint, myUtilCallback);
  Composer()(theComposerConf, myComputer.computer());

  std::unique_ptr<EdgeServerImpl> myComputerServerImpl;
  myComputerServerImpl.reset(
      new EdgeServerQuic(myComputer,
                         QuicParamsBuilder::buildServerHQParams(
                             theQuicServerConf, theServerEndpoint, 5)));
  myComputerServerImpl->run();

  EdgeClientQuic myClient(QuicParamsBuilder::buildClientHQParams(
      theQuicClientConf, theServerEndpoint));

  LambdaRequest myReq(
      "clambda0",
      std::string(10000, 'A')); //<<<< change sizes of LambdaRequest input
  LambdaResponse myResp = myClient.RunLambda(myReq, false);

  LOG(INFO) << myResp.toString();

  LOG(INFO) << "Terminating test\n";
}

} // namespace edge
} // namespace uiiit