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
#include "Edge/computer.h"
#include "Edge/edgeclientgrpc.h"
#include "Edge/edgecomputer.h"
#include "Edge/edgecomputerclient.h"
#include "Edge/edgecomputerserver.h"
#include "Edge/edgecontrollerclient.h"
#include "Edge/edgecontrollerflat.h"
#include "Edge/edgecontrollerrpc.h"
#include "Edge/edgecontrollerserver.h"
#include "Edge/edgedispatcher.h"
#include "Edge/edgerouter.h"
#include "Edge/edgeservergrpc.h"
#include "Edge/edgeserverimpl.h"
#include "Edge/forwardingtableserver.h"
#include "Edge/ptimeestimatorfactory.h"
#include "Support/chrono.h"
#include "Support/conf.h"

#include "gtest/gtest.h"

#include <deque>
#include <memory>
#include <string>

namespace uiiit {
namespace edge {

struct TestLambdaTransactionGrpc : public ::testing::Test {

  struct System {
    enum Type { ROUTER = 0, DISPATCHER = 1 };

    System(const Type aType, const std::string& aSubtype)
        : theControllerEndpoint("127.0.0.1:6475")
        , theRouterEndpoint("127.0.0.1:6473")
        , theForwardingEndpoint("127.0.0.1:6474")
        , theComputerEndpoint("127.0.0.1:10000")
        , theUtilEndpoint("127.0.0.1:6476")
        , theNumThreads(5)
        , theController(theControllerEndpoint)
        , theUtilServer(theUtilEndpoint)
        , theComputer(theComputerEndpoint,
                      [this](const std::map<std::string, double>& aUtil) {
                        theUtilServer.add(aUtil);
                      })
        , theRouter(aType == ROUTER ?
                        new EdgeRouter(
                            theRouterEndpoint,
                            theForwardingEndpoint,
                            theControllerEndpoint,
                            support::Conf(EdgeLambdaProcessor::defaultConf()),
                            support::Conf("type=random"),
                            support::Conf("type=trivial,period=10,stat=mean")) :
                        nullptr)
        , theDispatcher(
              aType == DISPATCHER ?
                  new EdgeDispatcher(
                      theRouterEndpoint,
                      theForwardingEndpoint,
                      theControllerEndpoint,
                      support::Conf(EdgeLambdaProcessor::defaultConf()),
                      support::Conf(
                          PtimeEstimatorFactory::defaultConf(aSubtype))) :
                  nullptr)
        , theForwardingTableServer(nullptr)
        , theUtils() {
      // configure the computer
      Composer()(
          support::Conf("type=intel-server,num-containers=1,num-workers=1"),
          theComputer.computer());

      // configure the controller
      std::unique_ptr<EdgeController> myEdgeControllerRpc;
      myEdgeControllerRpc.reset(new EdgeControllerRpc<EdgeControllerFlat>());
      theController.subscribe(std::move(myEdgeControllerRpc));

      // start all the servers (in a non-blocking fashion, obviously)
      theComputerServerImpl.reset(
          new EdgeServerGrpc(theComputer, theComputerEndpoint, theNumThreads));

      theController.run(false);
      theUtilServer.run(false);
      theComputerServerImpl->run();

      if (theRouter) {
        theEdgeServerImpl.reset(
            new EdgeServerGrpc(*theRouter, theRouterEndpoint, theNumThreads));
        theEdgeServerImpl->run();
        theForwardingTableServer.reset(
            new ForwardingTableServer(theForwardingEndpoint,
                                      *theRouter->tables()[0],
                                      *theRouter->tables()[1]));
      }
      if (theDispatcher) {
        theEdgeServerImpl.reset(new EdgeServerGrpc(
            *theDispatcher, theRouterEndpoint, theNumThreads));
        theEdgeServerImpl->run();
        theForwardingTableServer.reset(new ForwardingTableServer(
            theForwardingEndpoint, *theDispatcher->tables()[0]));
      }
      assert(theForwardingTableServer);
      theForwardingTableServer->run(false);

      // announce the computer to the controller
      EdgeControllerClient myControllerClient(theControllerEndpoint);
      myControllerClient.announceComputer(
          theComputerEndpoint, *theComputer.computer().containerList());

      // save utilization values in memory
      theUtilClient.reset(new EdgeComputerClient(theUtilEndpoint));
      theUtilThread = std::thread([this]() {
        try {
          theUtilClient->StreamUtil(
              [this](const std::string& aProcessor, const float aUtil) {
                assert(aProcessor == "xeon");
                theUtils.push_back(aUtil);
              });
        } catch (const std::exception& aErr) {
          LOG(WARNING) << "caught exception: " << aErr.what();
        }
      });

      LOG(INFO) << "starting test";
    }

    ~System() {
      LOG(INFO) << "terminating test";

      theUtilClient->cancel();

      assert(theUtilThread.joinable());
      theUtilThread.join();
    }

    const std::string theControllerEndpoint;
    const std::string theRouterEndpoint;
    const std::string theForwardingEndpoint;
    const std::string theComputerEndpoint;
    const std::string theUtilEndpoint;
    const size_t      theNumThreads;

    EdgeControllerServer                   theController;
    EdgeComputerServer                     theUtilServer;
    EdgeComputer                           theComputer;
    std::unique_ptr<EdgeServerImpl>        theComputerServerImpl;
    std::unique_ptr<EdgeRouter>            theRouter;
    std::unique_ptr<EdgeDispatcher>        theDispatcher;
    std::unique_ptr<EdgeServerImpl>        theEdgeServerImpl;
    std::unique_ptr<ForwardingTableServer> theForwardingTableServer;
    std::deque<float>                      theUtils;
    std::thread                            theUtilThread;
    std::unique_ptr<EdgeComputerClient>    theUtilClient;
  };
};

TEST_F(TestLambdaTransactionGrpc, test_grpc_endtoend) {
  const size_t N = 50;

  const std::list<System::Type> myTypes({System::ROUTER, System::DISPATCHER});

  const std::map<System::Type, std::set<std::string>> mySubtypes({
      {System::ROUTER, {""}},
      {System::DISPATCHER, PtimeEstimatorFactory::types()},
  });

  for (const auto& myType : myTypes) {
    const auto it = mySubtypes.find(myType);
    assert(it != mySubtypes.end());
    for (const auto& mySubtype : it->second) {
      LOG(INFO) << "*** Running end-to-end experiment with an "
                << (myType == System::ROUTER ? "edge router" :
                                               "edge dispatcher")
                << " " << mySubtype;

      System mySystem(myType, mySubtype);

      EdgeClientGrpc myClient(mySystem.theRouterEndpoint);
      LambdaRequest  myReq("clambda0", std::string(1000, 'A'));

      support::Chrono myChrono(false);
      for (size_t i = 0; i < N; i++) {
        myChrono.start();
        const auto myResp = myClient.RunLambda(myReq, false);
        ASSERT_EQ(mySystem.theComputerEndpoint, myResp.theResponder);
        ASSERT_EQ("OK", myResp.theRetCode);
        ASSERT_EQ(std::string(1000, 'A'), myResp.theOutput);
        ASSERT_EQ(0u, myResp.theDataOut.size());
        ASSERT_NEAR(myResp.theProcessingTime, myChrono.stop() * 1000, 10);
      }

      for (const auto& myUtil : mySystem.theUtils) {
        ASSERT_NEAR(0.05, myUtil, 0.01);
      }
    }
  }
}

} // namespace edge
} // namespace uiiit
