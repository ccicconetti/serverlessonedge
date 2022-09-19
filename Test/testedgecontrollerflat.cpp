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

#include "Edge/edgecontrollerflat.h"

#include "trivialedgecontrollerinstaller.h"

#include "gtest/gtest.h"

#include <map>
#include <set>
#include <string>
#include <utility>

namespace uiiit {
namespace edge {

struct TrivialEdgeControllerFlat final : public EdgeControllerFlat,
                                         public TrivialEdgeControllerInstaller {
  TrivialEdgeControllerFlat()
      : EdgeControllerFlat()
      , TrivialEdgeControllerInstaller()
      , theLambdas()
      , theComputers()
      , theDelComputerError(false) {
  }

  void addComputer(const std::string&            aEndpoint,
                   const std::list<std::string>& aLambdas) override {
    theComputers[aEndpoint] = aLambdas;
  }

  void delComputer(const std::string&            aEndpoint,
                   const std::list<std::string>& aLambdas) override {
    theDelComputerError = false;
    auto it             = theComputers.find(aEndpoint);
    if (it == theComputers.end()) {
      theDelComputerError = true;
      return;
    }
    if (it->second != aLambdas) {
      theDelComputerError = true;
    }
    theComputers.erase(it);
  }

  void addLambda(const std::string& aLambda) override {
    theLambdas.insert(aLambda);
  }

  void delLambda(const std::string& aLambda) override {
    theLambdas.erase(aLambda);
  }

  std::set<std::string>                         theLambdas;
  std::map<std::string, std::list<std::string>> theComputers;
  bool                                          theDelComputerError;
};

struct TestEdgeControllerFlat : public ::testing::Test {};

TEST_F(TestEdgeControllerFlat, test_ctor) {
  EXPECT_NO_THROW(TrivialEdgeControllerFlat{});
}

TEST_F(TestEdgeControllerFlat, test_add_computers_then_routers) {
  TrivialEdgeControllerFlat myController;

  EXPECT_TRUE(myController.theLambdas.empty());

  myController.announceComputer("host0:6473", makeContainers(2));
  myController.announceComputer("host1:6473", makeContainers(2));
  myController.announceComputer("host2:6473", makeContainers(1));
  EXPECT_EQ(std::string(""), myController.theLog);
  EXPECT_EQ(std::set<std::string>({"lambda0", "lambda1"}),
            myController.theLambdas);

  myController.announceRouter("host3:6473", "host3:6474");
  EXPECT_EQ(
      std::string("host3:6474: lambda0,host0:6473,1,F lambda1,host0:6473,1,F "
                  "lambda0,host1:6473,1,F lambda1,host1:6473,1,F "
                  "lambda0,host2:6473,1,F\n"),
      myController.theLog);

  myController.announceComputer("host4:6473", makeContainers(1));
  EXPECT_EQ(
      std::string("host3:6474: lambda0,host0:6473,1,F lambda1,host0:6473,1,F "
                  "lambda0,host1:6473,1,F lambda1,host1:6473,1,F "
                  "lambda0,host2:6473,1,F\n"
                  "host3:6474: lambda0,host4:6473,1,F\n"),
      myController.theLog);
}

TEST_F(TestEdgeControllerFlat, test_add_routers_then_computers) {
  TrivialEdgeControllerFlat myController;

  myController.announceRouter("host3:6473", "host3:6474");
  EXPECT_EQ(std::string(""), myController.theLog);
  EXPECT_TRUE(myController.theLambdas.empty());

  myController.announceComputer("host0:6473", makeContainers(2));
  myController.announceComputer("host1:6473", makeContainers(2));
  myController.announceComputer("host2:6473", makeContainers(1));
  EXPECT_EQ(
      std::string("host3:6474: lambda0,host0:6473,1,F lambda1,host0:6473,1,F\n"
                  "host3:6474: lambda0,host1:6473,1,F lambda1,host1:6473,1,F\n"
                  "host3:6474: lambda0,host2:6473,1,F\n"),
      myController.theLog);
  EXPECT_EQ(std::set<std::string>({"lambda0", "lambda1"}),
            myController.theLambdas);

  myController.announceRouter("host4:6473", "host4:6474");
  EXPECT_EQ(
      std::string("host3:6474: lambda0,host0:6473,1,F "
                  "lambda1,host0:6473,1,F\nhost3:6474: lambda0,host1:6473,1,F "
                  "lambda1,host1:6473,1,F\nhost3:6474: "
                  "lambda0,host2:6473,1,F\nhost4:6474: lambda0,host0:6473,1,F "
                  "lambda1,host0:6473,1,F lambda0,host1:6473,1,F "
                  "lambda1,host1:6473,1,F lambda0,host2:6473,1,F\n"),
      myController.theLog);
}

TEST_F(TestEdgeControllerFlat, test_add_duplicates) {
  TrivialEdgeControllerFlat myController;

  myController.announceRouter("host1:6473", "host1:6474");

  myController.announceComputer("host0:6473", makeContainers(1));
  EXPECT_EQ(std::string("host1:6474: lambda0,host0:6473,1,F\n"),
            myController.theLog);
  EXPECT_EQ(std::set<std::string>({"lambda0"}), myController.theLambdas);

  myController.announceComputer("host0:6473", makeContainers(1));
  EXPECT_EQ(std::string("host1:6474: lambda0,host0:6473,1,F\n"),
            myController.theLog);
  EXPECT_EQ(std::set<std::string>({"lambda0"}), myController.theLambdas);

  myController.theLog = std::string();
  myController.announceComputer("host0:6473", makeContainers(2));
  EXPECT_EQ("del host1:6474 host0:6473 lambda0\n"
            "host1:6474: lambda0,host0:6473,1,F lambda1,host0:6473,1,F\n",
            myController.theLog);
  EXPECT_EQ(std::set<std::string>({"lambda0", "lambda1"}),
            myController.theLambdas);

  myController.theLog = std::string();
  myController.announceRouter("host1:6473", "host1:6474");
  EXPECT_EQ("host1:6474: lambda0,host0:6473,1,F lambda1,host0:6473,1,F\n",
            myController.theLog);

  myController.theLog = std::string();
  myController.announceRouter("host1:6473", "host1:6475");
  EXPECT_EQ(std::string(
                "host1:6475: lambda0,host0:6473,1,F lambda1,host0:6473,1,F\n"),
            myController.theLog);

  myController.theLog = std::string();
  myController.announceRouter("host1:6476", "host1:6475");
  EXPECT_EQ("host1:6475: lambda0,host0:6473,1,F lambda1,host0:6473,1,F\n",
            myController.theLog);
}

TEST_F(TestEdgeControllerFlat, test_automatic_remove_routers) {
  TrivialEdgeControllerFlat myController;

  myController.announceRouter("host1:6473", "host1:6474");
  myController.announceRouter("host2:6473", "host2:6474");
  EXPECT_EQ(std::string(""), myController.theLog);

  myController.announceComputer("host3:6473", makeContainers(2));
  EXPECT_EQ(std::string(
                "host1:6474: lambda0,host3:6473,1,F lambda1,host3:6473,1,F\n"
                "host2:6474: lambda0,host3:6473,1,F lambda1,host3:6473,1,F\n"),
            myController.theLog);
  EXPECT_EQ(std::set<std::string>({"lambda0", "lambda1"}),
            myController.theLambdas);

  // disconnect router
  myController.theDisconnected = "host2:6474";
  myController.theLog          = std::string();
  myController.announceComputer("host3:6473", makeContainers(1, 0));
  EXPECT_EQ("del host1:6474 host3:6473 lambda0 lambda1\n"
            "host1:6474: lambda0,host3:6473,1,F\n",
            myController.theLog);
  EXPECT_EQ(std::set<std::string>({"lambda0"}), myController.theLambdas);

  // again
  myController.theLog = std::string();
  myController.announceComputer("host3:6473", makeContainers(1, 1));
  EXPECT_EQ("del host1:6474 host3:6473 lambda0\n"
            "host1:6474: lambda1,host3:6473,1,F\n",
            myController.theLog);
  EXPECT_EQ(std::set<std::string>({"lambda1"}), myController.theLambdas);
}

TEST_F(TestEdgeControllerFlat, test_automatic_remove_computer) {
  TrivialEdgeControllerFlat myController;

  myController.announceRouter("host1:6473", "host1:6474");
  myController.announceRouter("host2:6473", "host2:6474");
  EXPECT_EQ(std::string(""), myController.theLog);

  myController.announceComputer("host3:6473", makeContainers(1));
  myController.announceComputer("host4:6473", makeContainers(1));
  EXPECT_EQ(std::string("host1:6474: lambda0,host3:6473,1,F\n"
                        "host2:6474: lambda0,host3:6473,1,F\n"
                        "host1:6474: lambda0,host4:6473,1,F\n"
                        "host2:6474: lambda0,host4:6473,1,F\n"),
            myController.theLog);
  EXPECT_EQ(std::set<std::string>({"lambda0"}), myController.theLambdas);

  // remove a computer
  myController.theLog = std::string();
  myController.removeComputer("host4:6473");

  EXPECT_EQ(std::string("del host1:6474 host4:6473 lambda0\n"
                        "del host2:6474 host4:6473 lambda0\n"),
            myController.theLog);
  EXPECT_EQ(std::set<std::string>({"lambda0"}), myController.theLambdas);
}

TEST_F(TestEdgeControllerFlat, test_lambdas) {
  using CompMap = std::map<std::string, std::list<std::string>>;

  TrivialEdgeControllerFlat myController;
  EXPECT_TRUE(myController.theLambdas.empty());

  // new host, new lambda
  myController.announceComputer("host0:6473", makeContainers(1));
  EXPECT_EQ(std::set<std::string>({"lambda0"}), myController.theLambdas);
  EXPECT_EQ(CompMap({{"host0:6473", {"lambda0"}}}), myController.theComputers);
  EXPECT_FALSE(myController.theDelComputerError);

  // new host, same lambda
  myController.announceComputer("host1:6473", makeContainers(1));
  EXPECT_EQ(std::set<std::string>({"lambda0"}), myController.theLambdas);
  EXPECT_EQ(CompMap({
                {"host0:6473", {"lambda0"}},
                {"host1:6473", {"lambda0"}},
            }),
            myController.theComputers);
  EXPECT_FALSE(myController.theDelComputerError);

  // same host, same lambda
  myController.announceComputer("host1:6473", makeContainers(1));
  EXPECT_EQ(std::set<std::string>({"lambda0"}), myController.theLambdas);
  EXPECT_EQ(CompMap({
                {"host0:6473", {"lambda0"}},
                {"host1:6473", {"lambda0"}},
            }),
            myController.theComputers);
  EXPECT_FALSE(myController.theDelComputerError);

  // same host, new lambda
  myController.announceComputer("host1:6473", makeContainers(2));
  EXPECT_EQ(std::set<std::string>({"lambda0", "lambda1"}),
            myController.theLambdas);
  EXPECT_EQ(CompMap({
                {"host0:6473", {"lambda0"}},
                {"host1:6473", {"lambda0", "lambda1"}},
            }),
            myController.theComputers);
  EXPECT_FALSE(myController.theDelComputerError);

  // new host new lambda
  myController.announceComputer("host2:6473", makeContainers(3));
  EXPECT_EQ(std::set<std::string>({"lambda0", "lambda1", "lambda2"}),
            myController.theLambdas);
  EXPECT_EQ(CompMap({
                {"host0:6473", {"lambda0"}},
                {"host1:6473", {"lambda0", "lambda1"}},
                {"host2:6473", {"lambda0", "lambda1", "lambda2"}},
            }),
            myController.theComputers);
  EXPECT_FALSE(myController.theDelComputerError);

  // remove host, don't remove lambda
  myController.removeComputer("host0:6473");
  EXPECT_EQ(std::set<std::string>({"lambda0", "lambda1", "lambda2"}),
            myController.theLambdas);
  EXPECT_EQ(CompMap({
                {"host1:6473", {"lambda0", "lambda1"}},
                {"host2:6473", {"lambda0", "lambda1", "lambda2"}},
            }),
            myController.theComputers);
  EXPECT_FALSE(myController.theDelComputerError);

  // remove host, remove lambda
  myController.removeComputer("host2:6473");
  EXPECT_EQ(std::set<std::string>({"lambda0", "lambda1"}),
            myController.theLambdas);
  EXPECT_EQ(CompMap({
                {"host1:6473", {"lambda0", "lambda1"}},
            }),
            myController.theComputers);
  EXPECT_FALSE(myController.theDelComputerError);

  // remove last host, remove all lambdas
  myController.removeComputer("host1:6473");
  EXPECT_TRUE(myController.theLambdas.empty());
  EXPECT_TRUE(myController.theComputers.empty());
  EXPECT_FALSE(myController.theDelComputerError);
}

} // namespace edge
} // namespace uiiit
