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
        }) {
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
};

TEST_F(TestLambdaMuSim, test_scenario_from_files) {
  ASSERT_TRUE(prepareNetworkFiles(theTestDir));
  statesim::Network myNetwork((theTestDir / "nodes").string(),
                              (theTestDir / "links").string(),
                              (theTestDir / "edges").string());

  Scenario myScenario(
      myNetwork,
      [](const auto&) { return 2; },
      [](const auto&) { return 1.0; });
}

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
      [](const auto& aNode) { return 2; },
      [](const auto& aNode) { return 1.0; });

  // edge nodes have 2 containers each, with a lambda-capacity of 1
  // lambda-apps request a capacity of 1
  const auto myOut1 = myScenario.snapshot(6, 6, 0.5, 0.5, 1, 42);

  EXPECT_EQ(0, myOut1.theLambdaCost);
  EXPECT_EQ(12, myOut1.theMuCost);
  EXPECT_EQ(2, myOut1.theMuCloud);

  const auto myOut2 = myScenario.snapshot(6, 6, 1, 0.5, 1, 42);

  EXPECT_EQ(0, myOut2.theLambdaCost);
  EXPECT_EQ(7, myOut2.theMuCost);
  EXPECT_EQ(0, myOut2.theMuCloud);
}

} // namespace lambdamusim
} // namespace uiiit
