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

#include "Edge/edgelambdaprocessoroptions.h"
#include "Edge/edgerouter.h"
#include "Edge/edgeserverimpl.h"
#include "Quic/edgeclientquic.h"
#include "Quic/edgeserverquic.h"
#include "Quic/quicparamsbuilder.h"
#include "Support/conf.h"

#include "gtest/gtest.h"

#include <glog/logging.h>

#include <folly/ssl/Init.h>
#include <proxygen/lib/http/SynchronizedLruQuicPskCache.h>
#include <quic/QuicConstants.h>

namespace uiiit {
namespace edge {

struct TestEdgeServerQuic : public ::testing::Test {};

TEST_F(TestEdgeServerQuic, test_connection) {

  folly::ssl::init();

  HQParams myEdgeServerQuicParams =
      QuicParamsBuilder::build(support::Conf("transport-type=quic"), true);
  HQParams myEdgeClientQuicParams =
      QuicParamsBuilder::build(support::Conf("transport-type=quic"), false);

  std::unique_ptr<EdgeRouter> theRouter;
  LOG(INFO) << myEdgeServerQuicParams.host + ':' +
                   std::to_string(myEdgeServerQuicParams.port) + '\n';

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

  EdgeClientQuic myClient(myEdgeClientQuicParams);
  myClient.startClient();

  LambdaRequest  myReq("clambda0", std::string(50, 'A'));
  LambdaResponse myResp = myClient.RunLambda(myReq, false);
  // checks

  LOG(INFO) << myResp.toString();

  // myClient.RunLambda(myReq, false);

  LOG(INFO) << "Terminating test\n";
}

} // namespace edge
} // namespace uiiit