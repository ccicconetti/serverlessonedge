/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2021 C. Cicconetti <https://ccicconetti.github.io/>

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
#include "Edge/lambda.h"
#include "Edge/processortype.h"
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
      , theQuicServerConf("type=quic") {
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
  static const auto                   GB = uint64_t(1) << uint64_t(30);

  myUtilServer.reset(new EdgeComputerServer(theUtilEndpoint));
  myUtilCallback = [&myUtilServer](const std::map<std::string, double>& aUtil) {
    myUtilServer->add(aUtil);
  };
  myUtilServer->run(false); // non-blocking

  EdgeComputer myComputer(theServerEndpoint, myUtilCallback);

  // Composer()(theComposerConf, myComputer.computer());

  // the following snippet is equivalent to the previous LoC

  // add processors
  myComputer.computer().addProcessor(
      "arm", ProcessorType::GenericCpu, 1e11, 4, GB / 2); // 100x default speed
  myComputer.computer().addProcessor("bm2837",
                                     ProcessorType::GenericGpu,
                                     1e11, // 100x default speed
                                     1,
                                     GB / 2);

  // add containers, depending on the configuration
  auto myNumCpuContainers = theComposerConf.getUint("num-cpu-containers");
  while (myNumCpuContainers-- > 0) {
    const auto myNumCpuWorkers = theComposerConf.getUint("num-cpu-workers");
    const auto myId            = std::to_string(myNumCpuContainers);
    myComputer.computer().addContainer(
        "ccontainer" + myId,
        "arm",
        Lambda("clambda" + myId,
               ProportionalRequirements(1e6, 4 * 1e6, 100, 0)),
        myNumCpuWorkers);
  }

  auto myNumGpuContainers = theComposerConf.getUint("num-gpu-containers");
  while (myNumGpuContainers-- > 0) {
    const auto myNumGpuWorkers = theComposerConf.getUint("num-gpu-workers");
    const auto myId            = std::to_string(myNumGpuContainers);
    myComputer.computer().addContainer(
        "gcontainer" + myId,
        "bm2837",
        Lambda("glambda" + myId, ProportionalRequirements(1e6, 1e6, 100, 0)),
        myNumGpuWorkers);
  }

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