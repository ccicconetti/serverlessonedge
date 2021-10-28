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

#include "Edge/Model/chain.h"
#include "Edge/callbackserver.h"
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
#include "Edge/edgerouter.h"
#include "Edge/edgeservergrpc.h"
#include "Edge/edgeserverimpl.h"
#include "Edge/forwardingtableserver.h"
#include "Edge/lambda.h"
#include "Edge/processortype.h"
#include "Edge/stateclient.h"
#include "Edge/stateserver.h"
#include "Support/chrono.h"
#include "Support/conf.h"
#include "Support/wait.h"

#include "gtest/gtest.h"

#include <deque>
#include <memory>
#include <string>

namespace uiiit {
namespace edge {

struct TestChainTransactionGrpc : public ::testing::Test {
  const size_t N = 50;

  struct System {
    enum Type { ROUTER = 0, DISPATCHER = 1 };

    System()
        : theControllerEndpoint("127.0.0.1:6475")
        , theRouterEndpoint("127.0.0.1:6473")
        , theRouterCommandEndpoint("127.0.0.1:6474")
        , theStateServerEndpoint("127.0.0.1:6481")
        , theComputerEndpoints({"127.0.0.1:10001", "127.0.0.1:10002"})
        , theCompanionEndpoints({"127.0.0.1:10101", "127.0.0.1:10102"})
        , theComputerStateServerEndpoints(
              {"127.0.0.1:10201", "127.0.0.1:10202"})
        , theCompanionCommandEndpoints({"127.0.0.1:10301", "127.0.0.1:10302"})
        , theCallbackEndpoint("127.0.0.1:6480")
        , theNumThreads(5)
        , theController(theControllerEndpoint)
        , theRouter(theRouterEndpoint,
                    theRouterCommandEndpoint,
                    theControllerEndpoint,
                    support::Conf(EdgeLambdaProcessor::defaultConf()),
                    support::Conf("type=random"),
                    support::Conf("type=trivial,period=10,stat=mean"),
                    support::Conf("type=grpc,persistence=0.05"))
        , theRouterImpl(theRouter, theRouterEndpoint, theNumThreads)
        , theRouterCommand(theRouterCommandEndpoint,
                           *theRouter.tables()[0],
                           *theRouter.tables()[1])
        , theStateServer(theStateServerEndpoint) {
      static const auto GB = uint64_t(1) << uint64_t(30);
      assert(theComputerEndpoints.size() == theCompanionEndpoints.size());
      assert(theComputerEndpoints.size() ==
             theComputerStateServerEndpoints.size());
      assert(theComputerEndpoints.size() ==
             theCompanionCommandEndpoints.size());

      // configure the controller
      std::unique_ptr<EdgeController> myEdgeControllerRpc;
      myEdgeControllerRpc.reset(new EdgeControllerRpc<EdgeControllerFlat>());
      theController.subscribe(std::move(myEdgeControllerRpc));

      // create the computers and the companion servers
      for (size_t i = 0; i < theComputerEndpoints.size(); i++) {
        const auto myId = std::to_string(i);

        theComputers.emplace_back(std::make_unique<EdgeComputer>(
            theNumThreads, theComputerEndpoints[i], [](const auto&) {}));
        theComputers.back()->computer().addProcessor(
            "xeon", ProcessorType::GenericCpu, 4e9, 20, 128 * GB);
        theComputers.back()->computer().addContainer(
            "ccontainer" + myId,
            "xeon",
            Lambda("f" + myId, ProportionalRequirements(1e6, 4 * 1e6, 100, 0)),
            2);
        theComputerImpls.emplace_back(std::make_unique<EdgeServerGrpc>(
            *theComputers.back(), theComputerEndpoints[i], theNumThreads));
        theComputerStateServers.emplace_back(
            std::make_unique<StateServer>(theComputerStateServerEndpoints[i]));
        theCompanions.emplace_back(std::make_unique<EdgeRouter>(
            theCompanionEndpoints[i],
            theCompanionCommandEndpoints[i],
            theControllerEndpoint,
            support::Conf(EdgeLambdaProcessor::defaultConf()),
            support::Conf("type=random"),
            support::Conf("type=trivial,period=10,stat=mean"),
            support::Conf("type=grpc,persistence=0.05")));
        theCompanionImpls.emplace_back(std::make_unique<EdgeServerGrpc>(
            *theCompanions.back(), theCompanionEndpoints[i], theNumThreads));
        theCompanionCommands.emplace_back(
            std::make_unique<ForwardingTableServer>(
                theCompanionCommandEndpoints[i],
                *theCompanions.back()->tables()[0],
                *theCompanions.back()->tables()[0]));

        theComputers.back()->state(theComputerStateServerEndpoints[i]);
        theComputers.back()->companion(theCompanionEndpoints[i]);
      }

      // start all the servers (in a non-blocking fashion, obviously)
      theController.run(false);
      theRouterImpl.run();
      theRouterCommand.run(false);
      theStateServer.run(false);
      for (size_t i = 0; i < theComputerEndpoints.size(); i++) {
        theComputerImpls[i]->run();
        theComputerStateServers[i]->run(false);
        theCompanionImpls[i]->run();
        theCompanionCommands[i]->run(false);
      }

      // announce the computers to the controller
      EdgeControllerClient myControllerClient(theControllerEndpoint);
      for (size_t i = 0; i < theComputerEndpoints.size(); i++) {
        myControllerClient.announceComputer(
            theComputerEndpoints[i],
            *theComputers[i]->computer().containerList());
      }

      LOG(INFO) << "starting test";
    }

    ~System() {
      LOG(INFO) << "terminating test";
    }

    const std::string              theControllerEndpoint;
    const std::string              theRouterEndpoint;
    const std::string              theRouterCommandEndpoint;
    const std::string              theStateServerEndpoint;
    const std::vector<std::string> theComputerEndpoints;
    const std::vector<std::string> theCompanionEndpoints;
    const std::vector<std::string> theComputerStateServerEndpoints;
    const std::vector<std::string> theCompanionCommandEndpoints;
    const std::string              theCallbackEndpoint;
    const size_t                   theNumThreads;

    EdgeControllerServer                                theController;
    EdgeRouter                                          theRouter;
    EdgeServerGrpc                                      theRouterImpl;
    ForwardingTableServer                               theRouterCommand;
    StateServer                                         theStateServer;
    std::vector<std::unique_ptr<EdgeComputer>>          theComputers;
    std::vector<std::unique_ptr<EdgeServerImpl>>        theComputerImpls;
    std::vector<std::unique_ptr<StateServer>>           theComputerStateServers;
    std::vector<std::unique_ptr<EdgeRouter>>            theCompanions;
    std::vector<std::unique_ptr<EdgeServerImpl>>        theCompanionImpls;
    std::vector<std::unique_ptr<ForwardingTableServer>> theCompanionCommands;
  };
}; // namespace edge

TEST_F(TestChainTransactionGrpc, test_chain_correct) {
  System mySystem;

  EdgeClientGrpc myClient(mySystem.theRouterEndpoint);
  LambdaRequest  myReq("f0", std::string(10, 'A'));
  myReq.theCallback = mySystem.theCallbackEndpoint;
  myReq.states().emplace("s0", State::fromContent("content-state-0"));
  myReq.states().emplace("s1", State::fromContent("content-state-1"));
  myReq.theChain = std::make_unique<model::Chain>(
      model::Chain::Functions({"f0", "f1", "f0"}),
      model::Chain::Dependencies({
          {"s0", {"f0", "f1"}},
          {"s1", {"f1"}},
      }));
  myReq.theNextFunctionIndex = 0;
  CallbackServer::Queue myResponses;
  CallbackServer myCallbackServer(mySystem.theCallbackEndpoint, myResponses);
  myCallbackServer.run(false);

  for (size_t i = 0; i < N; i++) {
    const auto myResp = myClient.RunLambda(myReq, false);
    ASSERT_EQ("OK", myResp.theRetCode);
    ASSERT_TRUE(myResp.theAsynchronous);
  }

  ASSERT_TRUE(support::waitFor<size_t>(
      [&myResponses]() { return myResponses.size(); }, N, 10));

  for (size_t i = 0; i < N; i++) {
    const auto myResp = myResponses.pop();
    ASSERT_EQ("OK", myResp.theRetCode);
    ASSERT_FALSE(myResp.theAsynchronous);
    ASSERT_EQ(6, myResp.theHops);
    ASSERT_EQ(std::string(10, 'A'), myResp.theOutput);
    ASSERT_EQ(0u, myResp.theDataOut.size());
    ASSERT_EQ((std::map<std::string, State>({
                  {"s0", State::fromContent("content-state-0")},
                  {"s1", State::fromContent("content-state-1")},
              })),
              myResp.states());
  }

  // again with another chain
  myReq.theChain = std::make_unique<model::Chain>(
      model::Chain::Functions({"f1", "f1", "f0", "f1", "f1"}),
      model::Chain::Dependencies({
          {"s0", {"f0", "f1"}},
          {"s1", {"f1"}},
      }));

  for (size_t i = 0; i < N; i++) {
    const auto myResp = myClient.RunLambda(myReq, false);
    ASSERT_EQ("OK", myResp.theRetCode);
    ASSERT_TRUE(myResp.theAsynchronous);
  }

  ASSERT_TRUE(support::waitFor<size_t>(
      [&myResponses]() { return myResponses.size(); }, N, 10));

  for (size_t i = 0; i < N; i++) {
    const auto myResp = myResponses.pop();
    ASSERT_EQ("OK", myResp.theRetCode);
    ASSERT_FALSE(myResp.theAsynchronous);
    ASSERT_EQ(10, myResp.theHops);
    ASSERT_EQ(std::string(10, 'A'), myResp.theOutput);
    ASSERT_EQ(0u, myResp.theDataOut.size());
    ASSERT_EQ((std::map<std::string, State>({
                  {"s0", State::fromContent("content-state-0")},
                  {"s1", State::fromContent("content-state-1")},
              })),
              myResp.states());
  }
}

TEST_F(TestChainTransactionGrpc, test_chain_incorrect) {
  System mySystem;

  EdgeClientGrpc myClient(mySystem.theRouterEndpoint);
  LambdaRequest  myReq("f0", std::string(10, 'A'));
  myReq.theCallback = mySystem.theCallbackEndpoint;
  myReq.theChain    = std::make_unique<model::Chain>(
      model::Chain::Functions({"f0", "f1", "fX"}),
      model::Chain::Dependencies());
  myReq.theNextFunctionIndex = 0;
  CallbackServer::Queue myResponses;
  CallbackServer myCallbackServer(mySystem.theCallbackEndpoint, myResponses);
  myCallbackServer.run(false);

  for (size_t i = 0; i < N; i++) {
    const auto myResp = myClient.RunLambda(myReq, false);
    ASSERT_EQ("OK", myResp.theRetCode);
    ASSERT_TRUE(myResp.theAsynchronous);
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));
  ASSERT_EQ(0u, myResponses.size());

  // now send a good function invocation request
  myReq.theChain = std::make_unique<model::Chain>(
      model::Chain::Functions({"f0", "f1"}), model::Chain::Dependencies());
  const auto myResp = myClient.RunLambda(myReq, false);
  ASSERT_EQ("OK", myResp.theRetCode);
  ASSERT_TRUE(myResp.theAsynchronous);
  ASSERT_TRUE(support::waitFor<size_t>(
      [&myResponses]() { return myResponses.size(); }, 1, 1));
}

TEST_F(TestChainTransactionGrpc, test_single_function_async_remote_states) {
  System mySystem;
  assert(not mySystem.theComputerStateServerEndpoints.empty());

  // create the async responses collector
  CallbackServer::Queue myResponses;
  CallbackServer myCallbackServer(mySystem.theCallbackEndpoint, myResponses);
  myCallbackServer.run(false);

  // upload the states to the server
  StateClient myStateClient(mySystem.theStateServerEndpoint);
  ASSERT_NO_THROW(myStateClient.Put("s0", "content-s0"));
  ASSERT_NO_THROW(myStateClient.Put("s1", "content-s1"));

  // create the request
  LambdaRequest myReq("f0", std::string(10, 'A'));
  myReq.theCallback = mySystem.theCallbackEndpoint;
  myReq.states().emplace("s0",
                         State::fromLocation(mySystem.theStateServerEndpoint));
  myReq.states().emplace("s1",
                         State::fromLocation(mySystem.theStateServerEndpoint));
  myReq.states().emplace("sX",
                         State::fromLocation(mySystem.theStateServerEndpoint));
  myReq.theChain =
      std::make_unique<model::Chain>(model::Chain::Functions({"f0"}),
                                     model::Chain::Dependencies({
                                         {"s0", {"f0"}},
                                         {"s1", {"f0"}},
                                     }));

  // invoke the functions
  EdgeClientGrpc myClient(mySystem.theRouterEndpoint);
  for (size_t i = 0; i < N; i++) {
    const auto myAck = myClient.RunLambda(myReq, false);
    ASSERT_EQ("OK", myAck.theRetCode);
    ASSERT_TRUE(myAck.theAsynchronous);

    // wait for the response to be received
    ASSERT_TRUE(support::waitFor<size_t>(
        [&myResponses]() { return myResponses.size(); }, 1, 1));

    // copy state location from response to the next request
    const auto myResp = myResponses.pop();
    myReq.theStates   = myResp.theStates;
    ASSERT_EQ("OK", myResp.theRetCode);

    // check the received response
    ASSERT_FALSE(myResp.theAsynchronous);
    ASSERT_EQ(2, myResp.theHops);
    ASSERT_EQ(std::string(10, 'A'), myResp.theOutput);
    ASSERT_EQ(0u, myResp.theDataOut.size());
    ASSERT_EQ(
        (std::map<std::string, State>({
            {"s0",
             State::fromLocation(mySystem.theComputerStateServerEndpoints[0])},
            {"s1",
             State::fromLocation(mySystem.theComputerStateServerEndpoints[0])},
            {"sX", State::fromLocation(mySystem.theStateServerEndpoint)},
        })),
        myResp.states());
  }

  // check states
  StateClient myStateClient0(mySystem.theComputerStateServerEndpoints[0]);
  std::string myContent;
  ASSERT_TRUE(myStateClient0.Get("s0", myContent));
  ASSERT_EQ("content-s0", myContent);
  ASSERT_TRUE(myStateClient0.Get("s1", myContent));
  ASSERT_EQ("content-s1", myContent);
}

TEST_F(TestChainTransactionGrpc, test_single_function_sync_remote_states) {
  System mySystem;
  assert(not mySystem.theComputerStateServerEndpoints.empty());

  // upload the states to the server
  StateClient myStateClient(mySystem.theStateServerEndpoint);
  ASSERT_NO_THROW(myStateClient.Put("s0", "content-s0"));
  ASSERT_NO_THROW(myStateClient.Put("s1", "content-s1"));

  // create the request
  LambdaRequest myReq("f0", std::string(10, 'A'));
  myReq.states().emplace("s0",
                         State::fromLocation(mySystem.theStateServerEndpoint));
  myReq.states().emplace("s1",
                         State::fromLocation(mySystem.theStateServerEndpoint));
  myReq.states().emplace("sX",
                         State::fromLocation(mySystem.theStateServerEndpoint));
  myReq.theChain =
      std::make_unique<model::Chain>(model::Chain::Functions({"f0"}),
                                     model::Chain::Dependencies({
                                         {"s0", {"f0"}},
                                         {"s1", {"f0"}},
                                     }));

  // invoke the functions
  EdgeClientGrpc myClient(mySystem.theRouterEndpoint);
  for (size_t i = 0; i < N; i++) {
    const auto myResp = myClient.RunLambda(myReq, false);
    ASSERT_EQ("OK", myResp.theRetCode);
    ASSERT_FALSE(myResp.theAsynchronous);
    ASSERT_EQ(2, myResp.theHops);
    ASSERT_EQ(std::string(10, 'A'), myResp.theOutput);
    ASSERT_EQ(0u, myResp.theDataOut.size());
    ASSERT_EQ(
        (std::map<std::string, State>({
            {"s0",
             State::fromLocation(mySystem.theComputerStateServerEndpoints[0])},
            {"s1",
             State::fromLocation(mySystem.theComputerStateServerEndpoints[0])},
            {"sX", State::fromLocation(mySystem.theStateServerEndpoint)},
        })),
        myResp.states());

    // copy state location from response to the next request
    myReq.theStates = myResp.theStates;
  }

  // check states
  StateClient myStateClient0(mySystem.theComputerStateServerEndpoints[0]);
  std::string myContent;
  ASSERT_TRUE(myStateClient0.Get("s0", myContent));
  ASSERT_EQ("content-s0", myContent);
  ASSERT_TRUE(myStateClient0.Get("s1", myContent));
  ASSERT_EQ("content-s1", myContent);
}

TEST_F(TestChainTransactionGrpc, test_chain_remote_states) {
  System mySystem;
  assert(mySystem.theComputerStateServerEndpoints.size() >= 2);

  // create the async responses collector
  CallbackServer::Queue myResponses;
  CallbackServer myCallbackServer(mySystem.theCallbackEndpoint, myResponses);
  myCallbackServer.run(false);

  // upload the states to the server
  StateClient myStateClient(mySystem.theStateServerEndpoint);
  ASSERT_NO_THROW(myStateClient.Put("s0", "content-s0"));
  ASSERT_NO_THROW(myStateClient.Put("s1", "content-s1"));

  // create the request
  LambdaRequest myReq("f0", std::string(10, 'A'));
  myReq.theCallback = mySystem.theCallbackEndpoint;
  myReq.states().emplace("s0",
                         State::fromLocation(mySystem.theStateServerEndpoint));
  myReq.states().emplace("s1",
                         State::fromLocation(mySystem.theStateServerEndpoint));
  myReq.theChain = std::make_unique<model::Chain>(
      model::Chain::Functions({"f0", "f1", "f0"}),
      model::Chain::Dependencies({
          {"s0", {"f0", "f1"}},
          {"s1", {"f1"}},
      }));
  myReq.theNextFunctionIndex = 0;

  // invoke the functions
  EdgeClientGrpc myClient(mySystem.theRouterEndpoint);
  for (size_t i = 0; i < N; i++) {
    const auto myAck = myClient.RunLambda(myReq, false);
    ASSERT_EQ("OK", myAck.theRetCode);
    ASSERT_TRUE(myAck.theAsynchronous);

    // wait for the response to be received
    ASSERT_TRUE(support::waitFor<size_t>(
        [&myResponses]() { return myResponses.size(); }, 1, 1));

    // copy state location from response to the next request
    const auto myResp = myResponses.pop();
    myReq.theStates   = myResp.theStates;
    ASSERT_EQ("OK", myResp.theRetCode);

    // check the received response
    ASSERT_FALSE(myResp.theAsynchronous);
    ASSERT_EQ(6, myResp.theHops);
    ASSERT_EQ(std::string(10, 'A'), myResp.theOutput);
    ASSERT_EQ(0u, myResp.theDataOut.size());
    ASSERT_EQ(
        (std::map<std::string, State>({
            {"s0",
             State::fromLocation(mySystem.theComputerStateServerEndpoints[0])},
            {"s1",
             State::fromLocation(mySystem.theComputerStateServerEndpoints[1])},
        })),
        myResp.states());
  }

  // check states
  StateClient myStateClient0(mySystem.theComputerStateServerEndpoints[0]);
  StateClient myStateClient1(mySystem.theComputerStateServerEndpoints[1]);
  std::string myContent;
  ASSERT_TRUE(myStateClient0.Get("s0", myContent));
  ASSERT_FALSE(myStateClient1.Get("s0", myContent));
  ASSERT_EQ("content-s0", myContent);
  ASSERT_FALSE(myStateClient0.Get("s1", myContent));
  ASSERT_TRUE(myStateClient1.Get("s1", myContent));
  ASSERT_EQ("content-s1", myContent);
}

} // namespace edge
} // namespace uiiit
