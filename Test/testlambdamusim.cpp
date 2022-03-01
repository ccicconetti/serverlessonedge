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

#include <map>
#include <set>

namespace uiiit {
namespace lambdamusim {

#include "Test/Data/datastatesim.h"

struct TestLambdaMuSim : public ::testing::Test {
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
      [](const auto& aNode) { return aNode.name() == "X" ? 100 : 2; },
      [](const auto& aNode) { return aNode.name() == "X" ? 100 : 1.0; });

  const auto myOut = myScenario.snapshot(2, 2, 0.4, 0.6, 42, 1);

  ASSERT_EQ(0, myOut.theLambdaCost);
  ASSERT_EQ(0, myOut.theMuCost);
  ASSERT_EQ(0, myOut.theMuCloud);
}

} // namespace lambdamusim
} // namespace uiiit
