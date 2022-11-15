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
#include "LambdaMuSim/appmodel.h"
#include "LambdaMuSim/appperiods.h"
#include "LambdaMuSim/apppool.h"
#include "LambdaMuSim/scenario.h"
#include "LambdaMuSim/simulation.h"
#include "StateSim/link.h"
#include "StateSim/network.h"
#include "StateSim/node.h"
#include "Support/random.h"
#include "Support/tostring.h"

#include "gtest/gtest.h"

#include <glog/logging.h>

#include <google/protobuf/io/coded_stream.h>
#include <map>
#include <set>
#include <stdexcept>

namespace uiiit {
namespace lambdamusim {

#include "Test/Data/afdbdataset.h"
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

  AppModelConstant myAppModel(AppModel::Params{1, 1, 1});
  Scenario         myScenario(
      myNetwork,
      2.0,
      0.0,
      [](const auto& aNode) { return 2; },
      [](const auto& aNode) { return 1; },
      myAppModel);

  // edge nodes have 2 containers each, with a lambda-capacity of 1
  // lambda-apps request a capacity of 1
  const auto myOut1 = myScenario.snapshot(6, 6, 0.5, 1, 42);

  EXPECT_EQ(5 * 6 + 1 + 1 + 2, myOut1.theLambdaCost); // 5:cloud, 3: edge
  EXPECT_EQ(16, myOut1.theMuCost);
  EXPECT_EQ(2, myOut1.theMuCloud);
  EXPECT_EQ(8, myOut1.theNumLambda);
  EXPECT_EQ(5, myOut1.theNumMu);
  EXPECT_EQ(2 * 3 + 1 + 5, myOut1.theNumContainers);
  EXPECT_EQ(3 * 1 + 8, myOut1.theTotCapacity);
  EXPECT_EQ(0, myOut1.theMuMigrations);

  // repeat identical simulation
  const auto myOut1again = myScenario.snapshot(6, 6, 0.5, 1, 42);

  ASSERT_EQ(myOut1, myOut1again)
      << "\nexpected: " << ::toString(myOut1.toStrings(), ",")
      << "\nactual:   " << ::toString(myOut1again.toStrings(), ",");

  // same but use all edge containers for mu-apps
  const auto myOut2 = myScenario.snapshot(6, 6, 1, 1, 42);

  EXPECT_EQ(7 * 6 + 2, myOut2.theLambdaCost); // 7: cloud, 1: edge
  EXPECT_EQ(7, myOut2.theMuCost);
  EXPECT_EQ(0, myOut2.theMuCloud);

  // same but lambda-apps have bigger requests
  AppModelConstant myAnotherAppModel(AppModel::Params{2, 1, 1});
  myScenario.appModel(myAnotherAppModel);
  const auto myOut3 = myScenario.snapshot(6, 6, 1, 1, 42);

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
                        0.0,
                        "", // unused with snapshot
                        0,  // (ibidem)
                        0,  // (ibidem)
                        0,  // (ibidem)
                        0,  // (ibidem)
                        0,  // (ibidem)
                        10,
                        10,
                        0.5,
                        0.5,
                        "constant,1,1,1",
                        (theTestDir / "out").string(),
                        true},
                   42,
                   2);

  std::string myContent;
  std::getline(std::ifstream((theTestDir / "out").string()), myContent, '\0');
  VLOG(1) << '\n' << myContent;

  EXPECT_EQ("42,2.000000,0.000000,0.000000,0,0,10,10,0.500000,0.500000,"
            "constant,1,1,1,"
            "906,268,9."
            "000000,5.000000,21.000000,13.000000,0.000000,0,0\n"
            "43,2.000000,0.000000,0.000000,0,0,10,10,0.500000,0.500000,"
            "constant,1,1,1,"
            "911,276,13."
            "000000,10.000000,31.000000,31.000000,0.000000,0,0\n",
            myContent);

  // run again with same seed
  mySimulation.run(Conf{Conf::Type::Snapshot,
                        (theTestDir / "nodes").string(),
                        (theTestDir / "links").string(),
                        (theTestDir / "edges").string(),
                        2.0,
                        0.0,
                        "", // unused with snapshot
                        0,  // (ibidem)
                        0,  // (ibidem)
                        0,  // (ibidem)
                        0,  // (ibidem)
                        0,  // (ibidem)
                        10,
                        10,
                        0.5,
                        0.5,
                        "constant,1,1,1",
                        (theTestDir / "out").string(),
                        true},
                   43,
                   1);

  std::getline(std::ifstream((theTestDir / "out").string()), myContent, '\0');
  VLOG(1) << '\n' << myContent;

  EXPECT_EQ("42,2.000000,0.000000,0.000000,0,0,10,10,0.500000,0.500000,"
            "constant,1,1,1,"
            "906,268,9."
            "000000,5.000000,21.000000,13.000000,0.000000,0,0\n"
            "43,2.000000,0.000000,0.000000,0,0,10,10,0.500000,0.500000,"
            "constant,1,1,1,"
            "911,276,13."
            "000000,10.000000,31.000000,31.000000,0.000000,0,0\n"
            "43,2.000000,0.000000,0.000000,0,0,10,10,0.500000,0.500000,"
            "constant,1,1,1,"
            "911,276,13."
            "000000,10.000000,31.000000,31.000000,0.000000,0,0\n",
            myContent);
}

TEST_F(TestLambdaMuSim, test_app_pool) {
  const std::size_t     N = 20;
  AppPeriods            myAppPeriods(theDataset, theCostModel, 1);
  AppPool               myAppPool(myAppPeriods.periods(), N, 42);
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
  auto myCheckFloat = false;
  {
    support::UniformIntRv<std::size_t> myTestRv(0, 1000, 1, 2, 3);
    std::vector<std::size_t>           myTestNumbers(10);
    for (std::size_t i = 0; i < myTestNumbers.size(); i++) {
      myTestNumbers[i] = myTestRv();
    }
    std::vector<std::size_t> myExpectedNumbers(
        {398, 164, 146, 902, 617, 622, 988, 690, 204, 136});
    if (myTestNumbers == myExpectedNumbers) {
      myCheckFloat = true;
      LOG(INFO) << "also checking instance-specific numbers";
    }
  }
  LOG_IF(INFO, not myCheckFloat) << "only checking invariants";

  statesim::Network myNetwork(
      theExampleNodes, theExampleLinks, theExampleEdges, theExampleClients);

  AppModelConstant myAppModel(AppModel::Params{1, 1, 1});

  Scenario myScenario(
      myNetwork,
      2.0,
      0.0,
      [](const auto& aNode) { return 2; },
      [](const auto& aNode) { return 1; },
      myAppModel);

  // edge nodes have 2 containers each, with a lambda-capacity of 1
  // lambda-apps request a capacity of 1
  AppPeriods myAppPeriods(theDataset, theCostModel, 1);
  const auto myOut1 = myScenario.dynamic(
      10000, 0, 1000, myAppPeriods.periods(), 10, 0.5, 0.5, 42);

  if (myCheckFloat) {
    EXPECT_FLOAT_EQ(6.1521521, myOut1.theNumLambda);
    EXPECT_FLOAT_EQ(2.8478479, myOut1.theNumMu);
    EXPECT_EQ(16, myOut1.theNumContainers);
    EXPECT_EQ(21, myOut1.theTotCapacity);
    EXPECT_FLOAT_EQ(36.930931, myOut1.theLambdaCost);
    EXPECT_FLOAT_EQ(8.8438435, myOut1.theMuCost);
    EXPECT_FLOAT_EQ(0.6986987, myOut1.theMuCloud);
    EXPECT_EQ(15, myOut1.theMuMigrations);
    EXPECT_EQ(11, myOut1.theNumOptimizations);
  }

  // repeat identical simulation
  const auto myOut1again = myScenario.dynamic(
      10000, 0, 1000, myAppPeriods.periods(), 10, 0.5, 0.5, 42);

  ASSERT_EQ(myOut1, myOut1again)
      << "\nexpected: " << ::toString(myOut1.toStrings(), ",")
      << "\nactual:   " << ::toString(myOut1again.toStrings(), ",");

  // same but all mu-apps go to the cloud
  const auto myOut2 = myScenario.dynamic(
      10000, 0, 1000, myAppPeriods.periods(), 10, 0, 0.5, 42);

  EXPECT_EQ(myOut1.theNumLambda, myOut2.theNumLambda);
  EXPECT_EQ(myOut1.theNumMu, myOut2.theNumMu);
  EXPECT_EQ(myOut1.theNumContainers, myOut2.theNumContainers);
  EXPECT_EQ(myOut1.theTotCapacity, myOut2.theTotCapacity);
  EXPECT_EQ(myOut1.theLambdaCost, myOut2.theLambdaCost);
  if (myCheckFloat) {
    EXPECT_FLOAT_EQ(17.069069, myOut2.theMuCost);
    EXPECT_FLOAT_EQ(2.6006007, myOut2.theMuCloud);
  }
  EXPECT_EQ(0, myOut2.theMuMigrations);
  EXPECT_EQ(myOut1.theNumOptimizations, myOut2.theNumOptimizations);

  // same but with warm-up
  const auto myOut3 = myScenario.dynamic(
      10000, 0.2, 1000, myAppPeriods.periods(), 10, 0, 0.5, 42);

  EXPECT_EQ(myOut1.theNumLambda, myOut3.theNumLambda);
  EXPECT_EQ(myOut1.theNumMu, myOut3.theNumMu);
  EXPECT_EQ(myOut1.theNumContainers, myOut3.theNumContainers);
  EXPECT_EQ(myOut1.theTotCapacity, myOut3.theTotCapacity);
  EXPECT_EQ(myOut1.theLambdaCost, myOut3.theLambdaCost);
  if (myCheckFloat) {
    EXPECT_FLOAT_EQ(17.069069, myOut3.theMuCost);
    EXPECT_FLOAT_EQ(2.6006007, myOut3.theMuCloud);
  }
  EXPECT_EQ(0, myOut3.theMuMigrations);
  EXPECT_EQ(10, myOut3.theNumOptimizations);

  // same but with warm-up == duration
  const auto myOut4 = myScenario.dynamic(
      10000, 10000, 1000, myAppPeriods.periods(), 10, 0, 0.5, 42);

  EXPECT_TRUE(std::isnan(myOut4.theNumLambda));
  EXPECT_TRUE(std::isnan(myOut4.theNumMu));
  EXPECT_EQ(myOut1.theNumContainers, myOut4.theNumContainers);
  EXPECT_EQ(myOut1.theTotCapacity, myOut4.theTotCapacity);
  EXPECT_TRUE(std::isnan(myOut4.theLambdaCost));
  EXPECT_TRUE(std::isnan(myOut4.theMuCost));
  EXPECT_TRUE(std::isnan(myOut4.theMuCloud));
  EXPECT_EQ(0, myOut4.theMuMigrations);
  EXPECT_EQ(0, myOut4.theNumOptimizations);
}

TEST_F(TestLambdaMuSim, test_simulation_dynamic) {
  Simulation mySimulation(1);
  ASSERT_TRUE(prepareNetworkFiles(theTestDir));
  ASSERT_TRUE(prepareAfdbDatasetFiles(theTestDir));

  Conf myConf{Conf::Type::Dynamic,
              (theTestDir / "nodes").string(),
              (theTestDir / "links").string(),
              (theTestDir / "edges").string(),
              2.0,
              0.0,
              (theTestDir / "apps").string(),
              86400 * 1e3 * 9, // duration: 9 days
              3600 * 1e3 * 2,  // warm-up:  2 hours
              3600 * 1e3,      // epoch:    1 hour
              1,
              10,
              0, // unused with dynamic
              0, // (ibidem)
              0.5,
              0.5,
              "constant,1,1,1",
              (theTestDir / "out").string(),
              true};

  // run one replication
  mySimulation.run(myConf, 42, 1);

  std::string myContent;
  std::getline(std::ifstream((theTestDir / "out").string()), myContent, '\0');
  VLOG(1) << '\n' << myContent;

  EXPECT_EQ("42,2.000000,0.000000,3600000.000000,1,10,0,0,0.500000,0.500000,"
            "constant,1,"
            "1,1,910,268,7."
            "926733,1.073267,20.573380,2.645391,0.000000,2,9\n",
            myContent);

  // run two replications, starting with same seed as before
  mySimulation.run(myConf, 42, 2);

  std::getline(std::ifstream((theTestDir / "out").string()), myContent, '\0');
  VLOG(1) << '\n' << myContent;

  EXPECT_EQ(
      "42,2.000000,0.000000,3600000.000000,1,10,0,0,0.500000,0.500000,"
      "constant,1,1,1,910,268,7.926733,1.073267,20.573380,2.645391,0.000000,"
      "2,9\n"
      "42,2.000000,0.000000,3600000.000000,1,10,0,0,0.500000,0.500000,"
      "constant,1,1,1,910,268,7.926733,1.073267,20.573380,2.645391,0.000000,"
      "2,9\n"
      "43,2.000000,0.000000,3600000.000000,1,10,0,0,0.500000,0.500000,"
      "constant,1,1,1,914,276,11.926733,1.073267,30.779422,2.138962,0.000000,"
      "0,9\n",
      myContent);
}

TEST_F(TestLambdaMuSim, test_app_model_invalid) {
  ASSERT_THROW(makeAppModel(42, ""), std::runtime_error);
  ASSERT_THROW(makeAppModel(42, "unknown-type"), std::runtime_error);
}

TEST_F(TestLambdaMuSim, test_app_model_constant) {
  ASSERT_THROW(makeAppModel(42, "constant,12,1000,alpha"),
               std::invalid_argument);
  ASSERT_THROW(makeAppModel(42, "constant,12,1000"), std::runtime_error);
  ASSERT_THROW(makeAppModel(42, "constant,12"), std::runtime_error);
  ASSERT_THROW(makeAppModel(42, "constant"), std::runtime_error);

  auto myModel = makeAppModel(42, "constant,12,1000,420");

  for (auto i = 0; i < 10; i++) {
    const auto myParams = (*myModel)();
    ASSERT_EQ(12, myParams.theServiceRate);
    ASSERT_EQ(1000, myParams.theExchangeSize);
    ASSERT_EQ(420, myParams.theStorageSize);
  }
}

TEST_F(TestLambdaMuSim, test_app_model_classes) {
  ASSERT_THROW(makeAppModel(42, "classes"), std::runtime_error);
  ASSERT_THROW(makeAppModel(42, "constant,1,2"), std::runtime_error);
  ASSERT_THROW(makeAppModel(42, "constant,1,2,3,4,5"), std::runtime_error);

  // single class
  auto mySingleModel = makeAppModel(42, "classes,99,12,1000,420");

  for (auto i = 0; i < 10; i++) {
    const auto myParams = (*mySingleModel)();
    ASSERT_EQ(12, myParams.theServiceRate);
    ASSERT_EQ(1000, myParams.theExchangeSize);
    ASSERT_EQ(420, myParams.theStorageSize);
  }

  // two classes
  auto myDualModel = makeAppModel(42, "classes,1,12,1000,420,2,6,500,210");

  auto myFirst  = 0.0;
  auto myDouble = 0.0;
  for (auto i = 0; i < 1000; i++) {
    const auto myParams = (*myDualModel)();
    if (myParams.theServiceRate == 12) {
      ++myFirst;
      ASSERT_EQ(1000, myParams.theExchangeSize);
      ASSERT_EQ(420, myParams.theStorageSize);
    } else if (myParams.theServiceRate == 6) {
      ++myDouble;
      ASSERT_EQ(500, myParams.theExchangeSize);
      ASSERT_EQ(210, myParams.theStorageSize);
    } else {
      FAIL() << "invalid service rate " << myParams.theServiceRate;
    }
  }
  ASSERT_GE(myFirst, 0);
  ASSERT_NEAR(myDouble / myFirst, 2, 0.1)
      << "first = " << myFirst << ", double = " << myDouble;
}

} // namespace lambdamusim
} // namespace uiiit
