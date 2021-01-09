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
#include "Edge/edgeservergrpc.h"
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

struct TestEdgeClientMulti : public ::testing::Test {
  TestEdgeClientMulti()
      : theEndpoint1("localhost:10000")
      , theEndpoint2("localhost:10001")
      , theEndpoint3("localhost:10002")
      , theGrpcClientConf("transport-type=grpc,persistence=0.5")
      , theQuicClientConf("transport-type=quic,persistence=0.5")
      , theQuicServerConf1("type=quic,h2port=6667,httpServerThreads=1")
      , theQuicServerConf2("type=quic,h2port=6668,httpServerThreads=1")
      , theQuicServerConf3("type=quic,h2port=6669,httpServerThreads=1") {
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

  const std::string   theEndpoint1;
  const std::string   theEndpoint2;
  const std::string   theEndpoint3;
  const support::Conf theGrpcClientConf;
  const support::Conf theQuicClientConf;
  const support::Conf theQuicServerConf1;
  const support::Conf theQuicServerConf2;
  const support::Conf theQuicServerConf3;
};

TEST_F(TestEdgeClientMulti, test_ctor) {
  ASSERT_NO_THROW(EdgeClientMulti({theEndpoint1}, theGrpcClientConf));
  ASSERT_THROW(EdgeClientMulti({}, theGrpcClientConf), std::runtime_error);

  ASSERT_NO_THROW(EdgeClientMulti({theEndpoint1}, theQuicClientConf));
  ASSERT_THROW(EdgeClientMulti({}, theQuicClientConf), std::runtime_error);
}

TEST_F(TestEdgeClientMulti, test_grpc_one_destination) {
  EdgeClientMulti myGrpcClient({theEndpoint1}, theGrpcClientConf);

  auto myComputer = makeComputer(theEndpoint1);

  // exec lambda before the computer exists: failure
  LambdaRequest myReq("lambda0", "hello");
  ASSERT_NE("OK", myGrpcClient.RunLambda(myReq, false).theRetCode);
  ASSERT_NE("OK", myGrpcClient.RunLambda(myReq, true).theRetCode);

  // start computer: now lambda exec succeeds
  std::unique_ptr<EdgeServerImpl> myComputerEdgeServerImpl;
  myComputerEdgeServerImpl.reset(
      new EdgeServerGrpc(*myComputer, theEndpoint1, 1));
  myComputerEdgeServerImpl->run();

  ASSERT_TRUE(support::waitFor<std::string>(
      [&]() { return myGrpcClient.RunLambda(myReq, false).theRetCode; },
      "OK",
      1.0));
  ASSERT_EQ(theEndpoint1, myGrpcClient.RunLambda(myReq, false).theResponder);
  ASSERT_EQ("OK", myGrpcClient.RunLambda(myReq, true).theRetCode);
  ASSERT_EQ(theEndpoint1, myGrpcClient.RunLambda(myReq, true).theResponder);

  // bad lambda name: failure
  LambdaRequest myReqBad("lambdaXXX", "");
  ASSERT_NE("OK", myGrpcClient.RunLambda(myReqBad, false).theRetCode);
  ASSERT_NE("OK", myGrpcClient.RunLambda(myReqBad, true).theRetCode);
}

TEST_F(TestEdgeClientMulti, test_quic_one_destination) {
  EdgeClientMulti myQuicClient({theEndpoint1}, theQuicClientConf);

  auto myComputer = makeComputer(theEndpoint1);

  // exec lambda before the computer exists: failure
  LambdaRequest myReq("lambda0", "hello");
  ASSERT_NE("OK", myQuicClient.RunLambda(myReq, false).theRetCode);
  ASSERT_NE("OK", myQuicClient.RunLambda(myReq, true).theRetCode);

  // start computer: now lambda exec succeeds
  std::unique_ptr<EdgeServerImpl> myComputerEdgeServerImpl;
  myComputerEdgeServerImpl.reset(new EdgeServerQuic(
      *myComputer,
      QuicParamsBuilder::build(theQuicServerConf1, theEndpoint1, true)));
  myComputerEdgeServerImpl->run();

  ASSERT_TRUE(support::waitFor<std::string>(
      [&]() { return myQuicClient.RunLambda(myReq, false).theRetCode; },
      "OK",
      1.0));
  ASSERT_EQ(theEndpoint1, myQuicClient.RunLambda(myReq, false).theResponder);
  ASSERT_EQ("OK", myQuicClient.RunLambda(myReq, true).theRetCode);
  ASSERT_EQ(theEndpoint1, myQuicClient.RunLambda(myReq, true).theResponder);

  // bad lambda name: failure
  LambdaRequest myReqBad("lambdaXXX", "");
  ASSERT_NE("OK", myQuicClient.RunLambda(myReqBad, false).theRetCode);
  ASSERT_NE("OK", myQuicClient.RunLambda(myReqBad, true).theRetCode);
}

TEST_F(TestEdgeClientMulti, test_grpc_three_destinations) {
  // create three computers, one with slower speed
  auto myComputer1 = makeComputer(theEndpoint1, 1e9);
  auto myComputer2 = makeComputer(theEndpoint2, 1e9);
  auto myComputer3 = makeComputer(theEndpoint3, 1e8);

  // create three EdgeServerGrpc to handle lambda requests
  std::unique_ptr<EdgeServerImpl> myComputerEdgeServerImpl1;
  std::unique_ptr<EdgeServerImpl> myComputerEdgeServerImpl2;
  std::unique_ptr<EdgeServerImpl> myComputerEdgeServerImpl3;

  myComputerEdgeServerImpl1.reset(
      new EdgeServerGrpc(*myComputer1, theEndpoint1, 1));
  myComputerEdgeServerImpl2.reset(
      new EdgeServerGrpc(*myComputer2, theEndpoint2, 1));
  myComputerEdgeServerImpl3.reset(
      new EdgeServerGrpc(*myComputer3, theEndpoint3, 1));

  myComputerEdgeServerImpl1->run();
  myComputerEdgeServerImpl2->run();
  myComputerEdgeServerImpl3->run();

  // create the multi-client
  EdgeClientMulti myClient({theEndpoint1, theEndpoint2, theEndpoint3},
                           theGrpcClientConf);

  // wait for the fast computers to be ready and set
  std::set<std::string> myResponders;
  LambdaRequest         myReq("lambda0", "hello");
  ASSERT_TRUE(support::waitFor<bool>(
      [&]() {
        const auto ret = myClient.RunLambda(myReq, false);
        if (ret.theRetCode == "OK") {
          myResponders.insert(ret.theResponder);
        }
        return myResponders.size() == 2;
      },
      true,
      1.0))
      << myResponders.size();
  ASSERT_EQ(std::set<std::string>({theEndpoint1, theEndpoint2}), myResponders);

  // make sure also the slow computer is ready
  EdgeClientMulti myAnotherClient({theEndpoint3}, theGrpcClientConf);
  ASSERT_TRUE(support::waitFor<std::string>(
      [&]() { return myAnotherClient.RunLambda(myReq, false).theRetCode; },
      "OK",
      1.0))
      << myResponders.size();

  // execute 100 lambdas, check that are served evenly by the fast computers
  std::map<std::string, size_t> myCounter;
  for (size_t i = 0; i < 100; i++) {
    const auto ret = myClient.RunLambda(myReq, false);
    ASSERT_EQ("OK", ret.theRetCode);
    myCounter[ret.theResponder]++;
  }

  ASSERT_EQ(2u, myCounter.size());
  ASSERT_EQ(1u, myCounter.count(theEndpoint1));
  ASSERT_EQ(1u, myCounter.count(theEndpoint2));

  const auto myDelta = myCounter[theEndpoint1] > myCounter[theEndpoint2] ?
                           (myCounter[theEndpoint1] - myCounter[theEndpoint2]) :
                           (myCounter[theEndpoint2] - myCounter[theEndpoint1]);

  LOG(INFO) << "delta = " << myDelta;

  // both destinations are used evenly
  // ASSERT_LT(myDelta, 20);

  // both destinations are used
  ASSERT_GT(myCounter[theEndpoint1], 0u);
  ASSERT_GT(myCounter[theEndpoint2], 0u);
}

// TEST_F(TestEdgeClientMulti, test_quic_three_destinations) {
// create three computers, one with slower speed
// auto myComputer1 = makeComputer(theEndpoint1, 1e9);
// auto myComputer2 = makeComputer(theEndpoint2, 1e9);
// auto myComputer3 = makeComputer(theEndpoint3, 1e8);

// create three EdgeServerGrpc to handle lambda requests
// std::unique_ptr<EdgeServerImpl> myComputerEdgeServerImpl1;
// std::unique_ptr<EdgeServerImpl> myComputerEdgeServerImpl2;
// std::unique_ptr<EdgeServerImpl> myComputerEdgeServerImpl3;

// myComputerEdgeServerImpl1.reset(new EdgeServerQuic(
//     *myComputer1,
//     QuicParamsBuilder::build(theQuicServerConf1, theEndpoint1, true)));
// myComputerEdgeServerImpl2.reset(new EdgeServerQuic(
//     *myComputer2,
//     QuicParamsBuilder::build(theQuicServerConf2, theEndpoint2, true)));
// myComputerEdgeServerImpl3.reset(new EdgeServerQuic(
//     *myComputer3,
//     QuicParamsBuilder::build(theQuicServerConf3, theEndpoint3, true)));

// myComputerEdgeServerImpl1->run();
// myComputerEdgeServerImpl2->run();
// myComputerEdgeServerImpl3->run();

//   std::this_thread::sleep_for(std::chrono::seconds(1));

// create the multi-client
// EdgeClientMulti myClient({theEndpoint1, theEndpoint2, theEndpoint3},
//                          theQuicClientConf);

// wait for the fast computers to be ready and set
// std::set<std::string> myResponders;
// LambdaRequest         myReq("lambda0", "hello");
// ASSERT_TRUE(support::waitFor<bool>(
//     [&]() {
//       const auto ret = myClient.RunLambda(myReq, false);
//       if (ret.theRetCode == "OK") {
//         myResponders.insert(ret.theResponder);
//       }
//       return myResponders.size() == 2;
//     },
//     true,
//     1.0))
//     << myResponders.size();
//   ASSERT_EQ(std::set<std::string>({theEndpoint1, theEndpoint2}),
//   myResponders);

// // make sure also the slow computer is ready
// EdgeClientMulti myAnotherClient({theEndpoint3}, theQuicClientConf);
// ASSERT_TRUE(support::waitFor<std::string>(
//     [&]() { return myAnotherClient.RunLambda(myReq, false).theRetCode; },
//     "OK",
//     1.0))
//     << myResponders.size();

// // execute 100 lambdas, check that are served evenly by the fast computers
// std::map<std::string, size_t> myCounter;
// for (size_t i = 0; i < 100; i++) {
//   const auto ret = myClient.RunLambda(myReq, false);
//   ASSERT_EQ("OK", ret.theRetCode);
//   myCounter[ret.theResponder]++;
// }

// ASSERT_EQ(2u, myCounter.size());
// ASSERT_EQ(1u, myCounter.count(theEndpoint1));
// ASSERT_EQ(1u, myCounter.count(theEndpoint2));

// const auto myDelta = myCounter[theEndpoint1] > myCounter[theEndpoint2] ?
//                          (myCounter[theEndpoint1] -
//                          myCounter[theEndpoint2]) :
//                          (myCounter[theEndpoint2] -
//                          myCounter[theEndpoint1]);

// LOG(INFO) << "delta = " << myDelta;

// // both destinations are used evenly
// // ASSERT_LT(myDelta, 20);

// // both destinations are used
// ASSERT_GT(myCounter[theEndpoint1], 0u);
// ASSERT_GT(myCounter[theEndpoint2], 0u);
//}

} // namespace edge
} // namespace uiiit
