/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2022 C. Cicconetti <https://ccicconetti.github.io/>

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

#include "Dataset/afdb-utils.h"
#include "LambdaMuSim/apppool.h"
#include "LambdaMuSim/scenario.h"
#include "LambdaMuSim/simulation.h"
#include "StateSim/link.h"
#include "StateSim/network.h"
#include "StateSim/node.h"
#include "Support/tostring.h"

#include "gtest/gtest.h"

#include <glog/logging.h>

#include <google/protobuf/io/coded_stream.h>
#include <map>
#include <set>

namespace uiiit {
namespace lambdamusim {

#include "Test/Data/datastatesim.h"

struct TestLambdaMuSim : public ::testing::Test {

  /*
    example network topology:

    C --- D --- E
    |     |
    A     B

    A, B:    brokers (client apps connect to them)
    C, D, E: edge nodes
  */

  TestLambdaMuSim()
      : theTestDir("TO_REMOVE_DIR")
      , theExampleNodes(
            {statesim::Node("A", 0, 1, 1, statesim::Affinity::Cpu, false),
             statesim::Node("B", 1, 1, 1, statesim::Affinity::Cpu, false),
             statesim::Node("C", 2, 1, 1, statesim::Affinity::Cpu, true),
             statesim::Node("D", 3, 1, 1, statesim::Affinity::Cpu, true),
             statesim::Node("E", 4, 1, 1, statesim::Affinity::Cpu, true)})
      , theExampleLinks({
            statesim::Link(statesim::Link::Type::Node, "link_A_C", 5, 1),
            statesim::Link(statesim::Link::Type::Node, "link_C_D", 6, 1),
            statesim::Link(statesim::Link::Type::Node, "link_B_D", 7, 1),
            statesim::Link(statesim::Link::Type::Node, "link_D_E", 8, 1),
        })
      , theExampleEdges({
            {"A", {"link_A_C"}},
            {"B", {"link_B_D"}},
            {"C", {"link_A_C", "link_C_D"}},
            {"D", {"link_C_D", "link_B_D", "link_D_E"}},
            {"E", {"link_D_E"}},
            {"link_A_C", {"A", "C"}},
            {"link_C_D", {"C", "D"}},
            {"link_B_D", {"B", "D"}},
            {"link_D_E", {"D", "E"}},
        })
      , theExampleClients({
            "A",
            "B",
        })
      , theDataset({
            {"app0",
             {
                 {100, true},
                 {110, true},
                 {1000, true},
                 {1110, true},
                 {1120, true},
                 {1140, true},
                 {1160, true},
                 {2120, true},
                 {2140, true},
                 {2160, true},
             }},
            {"app1",
             {
                 {100, true},
                 {150, true},
                 {250, true},
                 {300, true},
                 {400, true},
                 {999, true},
             }},
            {"app2",
             {
                 {100, true},
                 {110, true},
                 {140, true},
                 {170, true},
                 {180, true},
                 {200, true},
             }},
        })
      , theCostModel{1, 10, 0, 0, 0.3, 0, 0, 0} {
    // noop
  }

  void SetUp() {
    boost::filesystem::remove_all(theTestDir);
    boost::filesystem::create_directories(theTestDir);
  }

  void TearDown() {
    boost::filesystem::remove_all(theTestDir);
  }

  const boost::filesystem::path                      theTestDir;
  const std::set<statesim::Node>                     theExampleNodes;
  const std::set<statesim::Link>                     theExampleLinks;
  const std::map<std::string, std::set<std::string>> theExampleEdges;
  const std::set<std::string>                        theExampleClients;
  const dataset::TimestampDataset                    theDataset;
  const dataset::CostModel                           theCostModel;
};

TEST_F(TestLambdaMuSim, test_example_snapshot) {
  statesim::Network myNetwork(
      theExampleNodes, theExampleLinks, theExampleEdges, theExampleClients);
  for (const auto& myClient : myNetwork.clients()) {
    VLOG(1) << myClient->toString() << ", distance from processing nodes: "
            << ::toString(myNetwork.processing(),
                          " ",
                          [&myClient, &myNetwork](const auto& aServer) {
                            return std::to_string(
                                myNetwork.hops(*myClient, *aServer));
                          });
  }

  Scenario myScenario(
      myNetwork,
      2.0,
      [](const auto& aNode) { return 2; },
      [](const auto& aNode) { return 1; });

  // edge nodes have 2 containers each, with a lambda-capacity of 1
  // lambda-apps request a capacity of 1
  const auto myOut1 = myScenario.snapshot(6, 6, 0.5, 1, 1, 42);

  EXPECT_EQ(5 * 6 + 1 + 1 + 2, myOut1.theLambdaCost); // 5:cloud, 3: edge
  EXPECT_EQ(16, myOut1.theMuCost);
  EXPECT_EQ(2, myOut1.theMuCloud);
  EXPECT_EQ(8, myOut1.theNumLambda);
  EXPECT_EQ(5, myOut1.theNumMu);
  EXPECT_EQ(2 * 3 + 1 + 5, myOut1.theNumContainers);
  EXPECT_EQ(3 * 1 + 8, myOut1.theTotCapacity);

  // same but use all edge containers for mu-apps
  const auto myOut2 = myScenario.snapshot(6, 6, 1, 1, 1, 42);

  EXPECT_EQ(7 * 6 + 2, myOut2.theLambdaCost); // 7: cloud, 1: edge
  EXPECT_EQ(7, myOut2.theMuCost);
  EXPECT_EQ(0, myOut2.theMuCloud);

  // same but lambda-apps have bigger requests
  const auto myOut3 = myScenario.snapshot(6, 6, 1, 1, 2, 42);

  EXPECT_EQ(15 * 6 + 2, myOut3.theLambdaCost); // 7.5: cloud, 0.5: edge
  EXPECT_EQ(myOut2.theMuCost, myOut3.theMuCost);
  EXPECT_EQ(myOut2.theMuCloud, myOut3.theMuCloud);
}

TEST_F(TestLambdaMuSim, test_simulation_snapshot) {
  Simulation mySimulation(1);
  ASSERT_TRUE(prepareNetworkFiles(theTestDir));

  // run two replications
  mySimulation.run(Conf{Conf::Type::Snapshot,
                        (theTestDir / "nodes").string(),
                        (theTestDir / "links").string(),
                        (theTestDir / "edges").string(),
                        2.0,
                        "", // unused with snapshot
                        10,
                        10,
                        0.5,
                        0.5,
                        1,
                        (theTestDir / "out").string(),
                        true},
                   42,
                   2);

  std::string myContent;
  std::getline(std::ifstream((theTestDir / "out").string()), myContent, '\0');

  EXPECT_EQ("42,2.000000,10,10,0.500000,0.500000,1,"
            "906,259,9.000000,5.000000,21.000000,13.000000,0.000000\n"
            "43,2.000000,10,10,0.500000,0.500000,1,"
            "911,263,13.000000,10.000000,31.000000,31.000000,0.000000\n",
            myContent);

  // run again with same seed
  mySimulation.run(Conf{Conf::Type::Snapshot,
                        (theTestDir / "nodes").string(),
                        (theTestDir / "links").string(),
                        (theTestDir / "edges").string(),
                        2.0,
                        "", // unused with snapshot
                        10,
                        10,
                        0.5,
                        0.5,
                        1,
                        (theTestDir / "out").string(),
                        true},
                   43,
                   1);

  std::getline(std::ifstream((theTestDir / "out").string()), myContent, '\0');

  EXPECT_EQ("42,2.000000,10,10,0.500000,0.500000,1,"
            "906,259,9.000000,5.000000,21.000000,13.000000,0.000000\n"
            "43,2.000000,10,10,0.500000,0.500000,1,"
            "911,263,13.000000,10.000000,31.000000,31.000000,0.000000\n"
            "43,2.000000,10,10,0.500000,0.500000,1,"
            "911,263,13.000000,10.000000,31.000000,31.000000,0.000000\n",
            myContent);

  VLOG(2) << '\n' << myContent;
}

TEST_F(TestLambdaMuSim, test_app_pool) {
  const std::size_t     N = 20;
  AppPool               myAppPool(theDataset, theCostModel, 1, N, 42);
  std::set<std::size_t> myAllApps;
  for (std::size_t i = 0; i < N; i++) {
    myAllApps.insert(i);
  }

  std::set<std::size_t> mySelected;
  double                myExpected = myAppPool.next();
  for (auto i = 0; i < 10000; i++) {
    std::size_t myApp;
    double      myTime;
    std::tie(myApp, myTime) = myAppPool.advance();
    ASSERT_LT(myTime, 1000);
    ASSERT_EQ(myExpected, myTime);
    mySelected.insert(myApp);
    myExpected = myAppPool.next();
  }
  ASSERT_EQ(myAllApps, mySelected);

  double myNext = 0;
  while (myNext == 0) {
    myAppPool.advance();
    myNext = myAppPool.next();
  }
  myAppPool.advance(myNext / 3);
  ASSERT_FLOAT_EQ(myNext * 2 / 3, myAppPool.next());
  ASSERT_FLOAT_EQ(myNext * 2 / 3, myAppPool.advance().second);
}

TEST_F(TestLambdaMuSim, test_example_dynamic) {
  statesim::Network myNetwork(
      theExampleNodes, theExampleLinks, theExampleEdges, theExampleClients);

  Scenario myScenario(
      myNetwork,
      2.0,
      [](const auto& aNode) { return 2; },
      [](const auto& aNode) { return 1; });

  // edge nodes have 2 containers each, with a lambda-capacity of 1
  // lambda-apps request a capacity of 1
  const auto myOut = myScenario.dynamic(
      10000, 1000, theDataset, theCostModel, 1, 10, 0.5, 0.5, 1, 42);

  EXPECT_FLOAT_EQ(4.4268537, myOut.theNumLambda);
  EXPECT_FLOAT_EQ(4.5731463, myOut.theNumMu);
  EXPECT_EQ(16, myOut.theNumContainers);
  EXPECT_EQ(12, myOut.theTotCapacity);
  EXPECT_FLOAT_EQ(26.546547, myOut.theLambdaCost);
  EXPECT_FLOAT_EQ(18.532532, myOut.theMuCost);
  EXPECT_FLOAT_EQ(4.4994993, myOut.theMuCloud);
  EXPECT_EQ(47, myOut.theMuMigrations);
}

} // namespace lambdamusim
} // namespace uiiit
