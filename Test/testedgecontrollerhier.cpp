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

#include "Edge/edgecontrollerhier.h"
#include "Edge/topology.h"

#include "trivialedgecontrollerinstaller.h"

#include "gtest/gtest.h"

#include <boost/filesystem.hpp>
#include <fstream>

namespace uiiit {
namespace edge {

struct TrivialEdgeControllerHier final : public EdgeControllerHier,
                                         public TrivialEdgeControllerInstaller {
  TrivialEdgeControllerHier()
      : EdgeControllerHier()
      , TrivialEdgeControllerInstaller() {
  }

  void addComputer(const std::string&, const std::list<std::string>&) override {
    // tested in TestEdgeControllerFlat
  }

  void delComputer(const std::string&, const std::list<std::string>&) override {
    // tested in TestEdgeControllerFlat
  }

  void addLambda(const std::string&) override {
    // tested in TestEdgeControllerFlat
  }

  void delLambda(const std::string&) override {
    // tested in TestEdgeControllerFlat
  }
};

struct TestEdgeControllerHier : public ::testing::Test {
  TestEdgeControllerHier()
      : theTopologyFilename("toremove_dist.txt") {
  }

  void TearDown() override {
    boost::filesystem::remove(theTopologyFilename);
  }

  /* create the following topology:
   *
   *   host0
   *        \
   *         +- host2 -- host3 -- host4
   *        /
   *   host1
   *
   * note the assignment of home e-routers does not change
   * regardless of whether we use a min-max or min-avg policy
   */
  std::unique_ptr<Topology> makeTopology() const {
    if (not boost::filesystem::is_regular(theTopologyFilename)) {
      boost::filesystem::remove_all(theTopologyFilename);
      std::ofstream myOutfile(theTopologyFilename);
      assert(myOutfile);
      myOutfile << "host0 0 2 1 2 3\n"
                   "host1 2 0 1 2 3\n"
                   "host2 1 1 0 1 2\n"
                   "host3 2 2 1 0 1\n"
                   "host4 3 3 2 1 0\n";
    }
    return std::make_unique<Topology>(theTopologyFilename);
  }

  void testRegular(const EdgeControllerHier::Objective aObjective) {
    // in the test we assume that the following services are created:
    //
    // host0: computer at port  10000 offering lambda0, lambda1
    // host0: computer at port  10001 offering lambda0
    // host1: computer at port  10000 offering lambda1
    // host2: router   at ports 6473/6474
    // host3: no services
    // host4: computer at port  10000 offering lambda0, lambda1
    // host4: router   at port  6473/6474
    // host4: router   at port  16473/16474
    //
    TrivialEdgeControllerHier myController;
    myController.loadTopology(makeTopology());
    myController.objective(aObjective);

    myController.announceRouter("host2:6473", "host2:6474");

    ASSERT_EQ("flush host2:6474\n", myController.logGetAndClear());

    myController.announceRouter("host4:6473", "host4:6474");

    ASSERT_EQ("flush host2:6474\n"
              "flush host4:6474\n",
              myController.logGetAndClear());

    myController.announceRouter("host4:16473", "host4:16474");

    ASSERT_EQ("flush host2:6474\n"
              "flush host4:16474\n"
              "flush host4:6474\n",
              myController.logGetAndClear());

    myController.announceComputer("host0:10000", makeContainers(2, 0));

    ASSERT_EQ("host2:6474: lambda0,host0:10000,1,F lambda1,host0:10000,1,F\n"
              "host4:6474: lambda0,host2:6473,1, lambda1,host2:6473,1,\n"
              "host4:16474: lambda0,host2:6473,1, lambda1,host2:6473,1,\n",
              myController.logGetAndClear());

    myController.announceComputer("host0:10001", makeContainers(1, 0));

    ASSERT_EQ("host2:6474: lambda0,host0:10001,1,F\n",
              myController.logGetAndClear());

    myController.announceComputer("host1:10000", makeContainers(1, 1));

    ASSERT_EQ("host2:6474: lambda1,host1:10000,1,F\n",
              myController.logGetAndClear());

    myController.announceComputer("host4:10000", makeContainers(2, 0));

    ASSERT_TRUE(
        myController.theLog ==
            "host4:6474: lambda0,host4:10000,1,F lambda1,host4:10000,1,F\n"
            "host2:6474: lambda0,host4:6473,1, lambda1,host4:6473,1,\n"
            "host4:16474: lambda0,host4:6473,1, lambda1,host4:6473,1,\n" or
        myController.theLog ==
            "host4:16474: lambda0,host4:10000,1,F lambda1,host4:10000,1,F\n"
            "host2:6474: lambda0,host4:16473,1, lambda1,host4:16473,1,\n"
            "host4:6474: lambda0,host4:16473,1, lambda1,host4:16473,1,\n")
        << myController.theLog;
    myController.theLog.clear();

    // now we force the removal of a router by disconnecting a forwarding table
    // server and announcing a new computer: we except all the forwarding tables
    // to be flushed and re-created from scratch
    myController.theDisconnected = "host4:16474";

    myController.announceComputer("host0:10002", makeContainers(1, 2));

    ASSERT_EQ("host2:6474: lambda2,host0:10002,1,F\n"
              "host4:6474: lambda2,host2:6473,1,\n"
              "flush host2:6474\n"
              "flush host4:6474\n"
              "host2:6474: lambda0,host0:10000,1,F lambda1,host0:10000,1,F\n"
              "host4:6474: lambda0,host2:6473,1, lambda1,host2:6473,1,\n"
              "host2:6474: lambda0,host0:10001,1,F\n"
              "host2:6474: lambda2,host0:10002,1,F\n"
              "host4:6474: lambda2,host2:6473,1,\n"
              "host2:6474: lambda1,host1:10000,1,F\n"
              "host4:6474: lambda0,host4:10000,1,F lambda1,host4:10000,1,F\n"
              "host2:6474: lambda0,host4:6473,1, lambda1,host4:6473,1,\n",
              myController.logGetAndClear());

    // now we add new router, which again should cause the full reset of the
    // forwarding tables on all the routers
    myController.announceRouter("host1:6473", "host1:6474");

    ASSERT_EQ("flush host1:6474\n"
              "flush host2:6474\n"
              "flush host4:6474\n"
              "host2:6474: lambda0,host0:10000,1,F lambda1,host0:10000,1,F\n"
              "host4:6474: lambda0,host2:6473,1, lambda1,host2:6473,1,\n"
              "host1:6474: lambda0,host2:6473,1, lambda1,host2:6473,1,\n"
              "host2:6474: lambda0,host0:10001,1,F\n"
              "host2:6474: lambda2,host0:10002,1,F\n"
              "host4:6474: lambda2,host2:6473,1,\n"
              "host1:6474: lambda2,host2:6473,1,\n"
              "host1:6474: lambda1,host1:10000,1,F\n"
              "host2:6474: lambda1,host1:6473,1,\n"
              "host4:6474: lambda1,host1:6473,1,\n"
              "host4:6474: lambda0,host4:10000,1,F lambda1,host4:10000,1,F\n"
              "host2:6474: lambda0,host4:6473,1, lambda1,host4:6473,1,\n"
              "host1:6474: lambda0,host4:6473,1, lambda1,host4:6473,1,\n",
              myController.logGetAndClear());

    // now let's remove the computers one at a time
    myController.removeComputer("host0:10000");
    ASSERT_EQ("del host2:6473 host0:10000 lambda0\n"
              "del host2:6473 host0:10000 lambda1\n"
              "del host1:6474 host2:6473 lambda1\n"
              "del host4:6474 host2:6473 lambda1\n",
              myController.logGetAndClear());

    myController.removeComputer("host0:10001");
    ASSERT_EQ("del host2:6473 host0:10001 lambda0\n"
              "del host1:6474 host2:6473 lambda0\n"
              "del host4:6474 host2:6473 lambda0\n",
              myController.logGetAndClear());

    myController.removeComputer("host0:10002");
    ASSERT_EQ("del host2:6473 host0:10002 lambda2\n"
              "del host1:6474 host2:6473 lambda2\n"
              "del host4:6474 host2:6473 lambda2\n",
              myController.logGetAndClear());

    myController.removeComputer("host1:10000");
    ASSERT_EQ("del host1:6473 host1:10000 lambda1\n"
              "del host2:6474 host1:6473 lambda1\n"
              "del host4:6474 host1:6473 lambda1\n",
              myController.logGetAndClear());

    myController.removeComputer("host4:10000");
    ASSERT_EQ("del host4:6473 host4:10000 lambda0\n"
              "del host1:6474 host4:6473 lambda0\n"
              "del host2:6474 host4:6473 lambda0\n"
              "del host4:6473 host4:10000 lambda1\n"
              "del host1:6474 host4:6473 lambda1\n"
              "del host2:6474 host4:6473 lambda1\n",
              myController.logGetAndClear());
  }

 private:
  const std::string theTopologyFilename;
};

TEST_F(TestEdgeControllerHier, test_ctor) {
  ASSERT_NO_THROW(TrivialEdgeControllerHier{});
}

TEST_F(TestEdgeControllerHier, test_dup_topology) {
  TrivialEdgeControllerHier myController;
  ASSERT_NO_THROW(myController.loadTopology(makeTopology()));
  ASSERT_THROW(myController.loadTopology(makeTopology()), std::runtime_error);
}

TEST_F(TestEdgeControllerHier, test_dup_objective) {
  TrivialEdgeControllerHier myController;
  ASSERT_NO_THROW(
      myController.objective(EdgeControllerHier::Objective::MinMax));
  ASSERT_THROW(myController.objective(EdgeControllerHier::Objective::MinMax),
               std::runtime_error);
}

TEST_F(TestEdgeControllerHier, test_cannot_announce_computer_without_topology) {
  TrivialEdgeControllerHier myController;
  myController.objective(EdgeControllerHier::Objective::MinMax);
  myController.announceRouter("host3:6473", "host3:6474");
  ASSERT_THROW(myController.announceComputer("host0:6473", makeContainers(1)),
               std::runtime_error);
  myController.loadTopology(makeTopology());
  ASSERT_NO_THROW(
      myController.announceComputer("host0:6473", makeContainers(1)));
}

TEST_F(TestEdgeControllerHier,
       test_cannot_announce_computer_without_objective) {
  TrivialEdgeControllerHier myController;
  myController.loadTopology(makeTopology());
  myController.announceRouter("host3:6473", "host3:6474");
  ASSERT_THROW(myController.announceComputer("host0:6473", makeContainers(1)),
               std::runtime_error);
  myController.objective(EdgeControllerHier::Objective::MinMax);
  ASSERT_NO_THROW(
      myController.announceComputer("host0:6473", makeContainers(1)));
}

TEST_F(TestEdgeControllerHier, test_unknown_address) {
  {
    TrivialEdgeControllerHier myController;
    myController.loadTopology(makeTopology());
    myController.objective(EdgeControllerHier::Objective::MinMax);
    ASSERT_NO_THROW(myController.announceRouter("host3:6473", "host3:6474"));
    ASSERT_THROW(
        myController.announceComputer("host666:6473", makeContainers(1)),
        std::runtime_error);
  }

  {
    TrivialEdgeControllerHier myController;
    myController.loadTopology(makeTopology());
    myController.objective(EdgeControllerHier::Objective::MinMax);
    ASSERT_NO_THROW(
        myController.announceComputer("host0:6473", makeContainers(1)));
    ASSERT_THROW(myController.announceRouter("host666:6473", "host666:6474"),
                 std::runtime_error);
  }
}

TEST_F(TestEdgeControllerHier, test_regular_minmax) {
  testRegular(EdgeControllerHier::Objective::MinMax);
}

TEST_F(TestEdgeControllerHier, test_regular_minavg) {
  testRegular(EdgeControllerHier::Objective::MinAvg);
}

TEST_F(TestEdgeControllerHier, test_all_routers_down) {
  TrivialEdgeControllerHier myController;
  myController.loadTopology(makeTopology());
  myController.objective(EdgeControllerHier::Objective::MinMax);

  const std::list<std::string> myHosts({
      "host0",
      "host1",
      "host2",
      "host3",
      "host4",
  });

  // announce two routers per node
  for (const auto& myHost : myHosts) {
    myController.announceRouter(myHost + ":6473", myHost + ":6474");
    myController.announceRouter(myHost + ":16473", myHost + ":16474");
  }

  // announce one computer
  myController.announceComputer("host0:10000", makeContainers(1, 0));
  myController.theLog = std::string();

  // make all routers unreachable
  myController.theDisconnected = "*";

  // announce one computer
  myController.announceComputer("host0:10001", makeContainers(1, 0));

  // nothing done
  ASSERT_EQ("", myController.logGetAndClear());

  // no more routers present in the controller
  std::stringstream myStream;
  myController.print(myStream);

  ASSERT_EQ("computer host0:10000\n"
            "cont0: processor cpu0, lambda lambda0, #workers 2\n"
            "computer host0:10001\n"
            "cont0: processor cpu0, lambda lambda0, #workers 2\n",
            myStream.str());
}

TEST_F(TestEdgeControllerHier, test_computer_changes_lambdas) {
  TrivialEdgeControllerHier myController;
  myController.loadTopology(makeTopology());
  myController.objective(EdgeControllerHier::Objective::MinMax);

  // two routers
  myController.announceRouter("host0:6473", "host0:6474");
  myController.announceRouter("host4:6473", "host4:6474");

  // one computer
  myController.announceComputer("host0:10000", makeContainers(1, 0));

  ASSERT_EQ("flush host0:6474\n"
            "flush host0:6474\n"
            "flush host4:6474\n"
            "host0:6474: lambda0,host0:10000,1,F\n"
            "host4:6474: lambda0,host0:6473,1,\n",
            myController.logGetAndClear());

  // change lambda on that computer
  myController.announceComputer("host0:10000", makeContainers(1, 1));

  ASSERT_EQ("del host0:6473 host0:10000 lambda0\n"
            "del host4:6474 host0:6473 lambda0\n"
            "host0:6474: lambda1,host0:10000,1,F\n"
            "host4:6474: lambda1,host0:6473,1,\n",
            myController.logGetAndClear());
}

} // namespace edge
} // namespace uiiit
