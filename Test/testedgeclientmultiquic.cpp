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

#include "Edge/computer.h"
#include "Edge/edgeclientmulti.h"
#include "Edge/edgecomputer.h"
#include "Edge/edgeserverimpl.h"
#include "Edge/lambda.h"
#include "Edge/processortype.h"
#include "Quic/edgeserverquic.h"
#include "Support/conf.h"
#include "Support/wait.h"

#include "gtest/gtest.h"

#include <glog/logging.h>

namespace uiiit {
namespace edge {

struct TestEdgeClientMultiQuic : public ::testing::Test {
  TestEdgeClientMultiQuic()
      : theEndpoint("localhost:10000")
      , theQuicClientConf("type=quic,persistence=0.5")
      , theQuicServerConf("type=quic") {
  }

  static std::unique_ptr<EdgeComputer>
  makeComputer(const std::string& aEndpoint, const double aCpuSpeed = 1e9) {
    auto ret =
        std::make_unique<EdgeComputer>(aEndpoint, Computer::UtilCallback());
    ret->computer().addProcessor(
        "cpu", ProcessorType::GenericCpu, aCpuSpeed, 1, 1);
    ret->computer().addContainer(
        "container",
        "cpu",
        Lambda("lambda0", ProportionalRequirements(1e6, 1e6, 0, 0)),
        1);
    return ret;
  }

  const std::string   theEndpoint;
  const support::Conf theQuicClientConf;
  const support::Conf theQuicServerConf;
};

TEST_F(TestEdgeClientMultiQuic, test_ctor) {
  ASSERT_NO_THROW(EdgeClientMulti({theEndpoint}, theQuicClientConf));
  ASSERT_THROW(EdgeClientMulti({}, theQuicClientConf), std::runtime_error);
}

TEST_F(TestEdgeClientMultiQuic, test_quic_one_destination) {
  EdgeClientMulti myQuicClient({theEndpoint}, theQuicClientConf);

  auto myComputer = makeComputer(theEndpoint);

  // exec lambda before the computer exists: failure
  LambdaRequest myReq("lambda0", "hello");
  ASSERT_NE("OK", myQuicClient.RunLambda(myReq, false).theRetCode);
  ASSERT_NE("OK", myQuicClient.RunLambda(myReq, true).theRetCode);

  // start computer: now lambda exec succeeds
  std::unique_ptr<EdgeServerImpl> myComputerEdgeServerImpl;
  myComputerEdgeServerImpl.reset(
      new EdgeServerQuic(*myComputer,
                         QuicParamsBuilder::buildServerHQParams(
                             theQuicServerConf, theEndpoint, 1)));
  myComputerEdgeServerImpl->run();

  ASSERT_TRUE(support::waitFor<std::string>(
      [&]() { return myQuicClient.RunLambda(myReq, false).theRetCode; },
      "OK",
      1.0));
  ASSERT_EQ(theEndpoint, myQuicClient.RunLambda(myReq, false).theResponder);
  ASSERT_EQ("OK", myQuicClient.RunLambda(myReq, true).theRetCode);
  ASSERT_EQ(theEndpoint, myQuicClient.RunLambda(myReq, true).theResponder);

  // bad lambda name: failure
  LambdaRequest myReqBad("lambdaNotExisting", "");
  ASSERT_NE("OK", myQuicClient.RunLambda(myReqBad, false).theRetCode);
  ASSERT_NE("OK", myQuicClient.RunLambda(myReqBad, true).theRetCode);
}

} // namespace edge
} // namespace uiiit
