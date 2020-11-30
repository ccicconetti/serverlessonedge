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

#include "gtest/gtest.h"

#include "Edge/composer.h"
#include "Edge/computer.h"
#include "Edge/edgecomputer.h"
#include "Edge/edgecomputerclient.h"
#include "Edge/edgecomputerserver.h"
#include "Edge/edgecontrollerclient.h"
#include "Edge/edgecontrolleretsibalanced.h"
#include "Edge/edgecontrollerserver.h"
#include "Edge/edgeservergrpc.h"
#include "Edge/edgeserverimpl.h"
#include "Edge/etsiedgeclient.h"
#include "EtsiMec/appcontextmanager.h"
#include "EtsiMec/applistclient.h"
#include "EtsiMec/grpcueapplcmproxy.h"
#include "EtsiMec/staticueapplcmproxy.h"
#include "Support/chrono.h"
#include "Support/conf.h"
#include "Support/tostring.h"
#include "Support/wait.h"

#include <memory>
#include <string>

namespace uiiit {
namespace edge {

struct TestEtsiTransaction : public ::testing::Test {

  struct System {
    System()
        : theControllerEndpoint("127.0.0.1:6475")
        , theProxyEndpoint("127.0.0.1:6477")
        , theComputer1Endpoint("127.0.0.1:10000")
        , theComputer2Endpoint("127.0.0.1:10001")
        , theApiRoot("http://127.0.0.1:6500")
        , theNumThreads(1)
        , theProxy(theApiRoot)
        , theProxyServer(theProxyEndpoint, theProxy)
        , theController(theControllerEndpoint)
        , theComputer1(theComputer1Endpoint,
                       [](const std::map<std::string, double>&) {})
        , theComputer2(theComputer2Endpoint,
                       [](const std::map<std::string, double>&) {}) {
      // configure the computers
      Composer()(
          support::Conf("type=intel-server,num-containers=1,num-workers=1"),
          theComputer1.computer());
      Composer()(
          support::Conf("type=intel-server,num-containers=1,num-workers=1"),
          theComputer2.computer());

      // start the LCM proxy first (non-blocking)
      theProxy.start();
      theProxyServer.run(false);

      // create the gRPC interface to the LCM proxy
      std::unique_ptr<EdgeController> myEdgeControllerEtsi(
          new EdgeControllerEtsiBalanced(theProxyEndpoint, 5.0));
      theController.subscribe(std::move(myEdgeControllerEtsi));

      // create the EdgeServerGrpc to handle lambda requests
      theServerImpl1.reset(
          new EdgeServerGrpc(theComputer1, theComputer1Endpoint, 1));
      theServerImpl2.reset(
          new EdgeServerGrpc(theComputer2, theComputer2Endpoint, 1));

      // then, start the edge servers (non-blocking)
      theController.run(false);
      theServerImpl1->run();
      theServerImpl2->run();

      // announce the computers to the controller
      EdgeControllerClient myControllerClient(theControllerEndpoint);
      myControllerClient.announceComputer(
          theComputer1Endpoint, *theComputer1.computer().containerList());
      myControllerClient.announceComputer(
          theComputer2Endpoint, *theComputer2.computer().containerList());

      LOG(INFO) << "starting test";
    }

    ~System() {
      LOG(INFO) << "terminating test";
    }

    const std::string theControllerEndpoint;
    const std::string theProxyEndpoint;
    const std::string theComputer1Endpoint;
    const std::string theComputer2Endpoint;
    const std::string theApiRoot;
    const size_t      theNumThreads;

    uiiit::etsimec::StaticUeAppLcmProxy theProxy;
    uiiit::etsimec::GrpcUeAppLcmProxy   theProxyServer;
    EdgeControllerServer                theController;
    EdgeComputer                        theComputer1;
    EdgeComputer                        theComputer2;
    std::unique_ptr<EdgeServerImpl>     theServerImpl1;
    std::unique_ptr<EdgeServerImpl>     theServerImpl2;
  };
};

TEST_F(TestEtsiTransaction, test_endtoend) {
  const std::string myNotificationUri("http://localhost:6600");
  System            mySystem;

  // check the list of applications exposed by the LCM proxy
  const auto myAppList = uiiit::etsimec::AppListClient(mySystem.theApiRoot)();
  ASSERT_EQ(1u, myAppList.size());
  ASSERT_EQ("clambda0", myAppList.front().appName());

  // create a LCM proxy client and verify that the edge router table has two
  // entries and no contexts
  uiiit::etsimec::GrpcUeAppLcmProxyClient myProxyClient(
      mySystem.theProxyEndpoint);
  auto myTable = myProxyClient.table();
  ASSERT_EQ(1u, myTable.size());
  ASSERT_EQ("", std::get<0>(myTable.front()));
  ASSERT_EQ("clambda0", std::get<1>(myTable.front()));
  ASSERT_EQ(mySystem.theComputer1Endpoint, std::get<2>(myTable.front()));
  ASSERT_EQ(0u, myProxyClient.numContexts());

  // create context and a client
  auto myContextManager = std::make_unique<uiiit::etsimec::AppContextManager>(
      myNotificationUri, mySystem.theApiRoot);
  myContextManager->start();
  EtsiEdgeClient myClient(*myContextManager);

  // run a lambda
  LambdaRequest myReq("clambda0", std::string(1000, 'A'));
  const auto    myResp1 = myClient.RunLambda(myReq, false);
  ASSERT_TRUE(myResp1.theResponder.empty());
  ASSERT_EQ("OK", myResp1.theRetCode);
  ASSERT_EQ(std::string(1000, 'A'), myResp1.theOutput);
  ASSERT_EQ(0u, myResp1.theDataOut.size());
  ASSERT_EQ(mySystem.theComputer1Endpoint,
            myContextManager->referenceUri(myClient.ueAppId("clambda0")));
  ASSERT_TRUE(myClient.ueAppId("non-lambda").empty());
  ASSERT_THROW(myContextManager->referenceUri("invalid-ue-app-id"),
               std::runtime_error);
  ASSERT_EQ(1u, myProxyClient.numContexts());

  // de-register computer1 and wait for a notification to the client
  EdgeControllerClient myControllerClient(mySystem.theControllerEndpoint);
  myControllerClient.removeComputer(mySystem.theComputer1Endpoint);

  ASSERT_TRUE(uiiit::support::waitFor<std::string>(
      [&]() {
        return myContextManager->referenceUri(myClient.ueAppId("clambda0"));
      },
      mySystem.theComputer2Endpoint,
      1.0));

  // verify that the LCM edge router table has changed
  myTable = myProxyClient.table();
  ASSERT_EQ(1u, myTable.size());
  ASSERT_EQ("", std::get<0>(myTable.front()));
  ASSERT_EQ("clambda0", std::get<1>(myTable.front()));
  ASSERT_EQ(mySystem.theComputer2Endpoint, std::get<2>(myTable.front()));

  // verify that the list of apps has not changed
  const auto myAnotherAppList =
      uiiit::etsimec::AppListClient(mySystem.theApiRoot)();
  ASSERT_EQ(myAnotherAppList.size(), myAppList.size());
  for (auto it = myAnotherAppList.begin(), jt = myAppList.begin();
       it != myAnotherAppList.end();
       ++it, ++jt) {
    ASSERT_EQ(toString(*it), toString(*jt));
  }

  // does not compile with gcc 7.5.0 ¯\_(ツ)_/¯
  // ASSERT_EQ(myAppList, myAnotherAppList)*/

  // run another lambda on the other computer
  const auto myResp2 = myClient.RunLambda(myReq, false);
  ASSERT_TRUE(myResp2.theResponder.empty());
  ASSERT_EQ("OK", myResp2.theRetCode);
  ASSERT_EQ(std::string(1000, 'A'), myResp2.theOutput);
  ASSERT_EQ(0u, myResp2.theDataOut.size());

  // re-register the second edge computer, the LCM edge router table should
  // become empty
  myControllerClient.removeComputer(mySystem.theComputer2Endpoint);
  ASSERT_TRUE(uiiit::support::waitFor<bool>(
      [&]() { return myProxyClient.table().empty(); }, true, 1.0));

  // delete the context manager and checks that the context disappears
  myContextManager.reset();
  ASSERT_TRUE(uiiit::support::waitFor<size_t>(
      [&]() { return myProxyClient.numContexts(); }, 0, 1.0));

  // the app list in the LCM proxy also becomes empty
  ASSERT_TRUE(uiiit::etsimec::AppListClient(mySystem.theApiRoot)().empty());
}

} // namespace edge
} // namespace uiiit
