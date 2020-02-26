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

#include "Edge/edgecontrolleretsibalanced.h"

namespace uiiit {
namespace edge {

struct TestEdgeControllerEtsiBalanced : public ::testing::Test {};

TEST_F(TestEdgeControllerEtsiBalanced, test_table) {
  using Table   = EdgeControllerEtsiBalanced::Table;
  using DescMap = EdgeControllerEtsiBalanced::DescMap;

  // empty table
  Table   myTableIn;
  DescMap myDesc;
  EXPECT_TRUE(EdgeControllerEtsiBalanced::table(myTableIn, myDesc).empty());

  // add one computer serving lambda0
  auto ret = myDesc.emplace("lambda0", DescMap::mapped_type{"comp0"});

  // active contexts = one client with that lambda
  myTableIn.emplace_back("client0", "lambda0", "unused");

  auto myTableOut = EdgeControllerEtsiBalanced::table(myTableIn, myDesc);
  EXPECT_EQ(1u, myTableOut.size());
  EXPECT_EQ("client0", std::get<0>(myTableOut.front()));
  EXPECT_EQ("lambda0", std::get<1>(myTableOut.front()));
  EXPECT_EQ("comp0", std::get<2>(myTableOut.front()));

  // add a few clients, they should all go to the (only) computer
  for (auto i = 1; i < 5; i++) {
    myTableIn.emplace_back("client" + std::to_string(i), "lambda0", "unused");
  }

  myTableOut = EdgeControllerEtsiBalanced::table(myTableIn, myDesc);
  EXPECT_EQ(5u, myTableOut.size());
  auto myCounter = 0;
  for (const auto& myRow : myTableOut) {
    EXPECT_EQ("client" + std::to_string(myCounter), std::get<0>(myRow));
    EXPECT_EQ("lambda0", std::get<1>(myRow));
    EXPECT_EQ("comp0", std::get<2>(myRow));
    ++myCounter;
  }

  // add a client whose lambda is not yet served, check it is ignored
  myTableIn.emplace_back("client5", "lambda1", "unused");
  EXPECT_EQ(myTableOut, EdgeControllerEtsiBalanced::table(myTableIn, myDesc));

  // add another computer, check they split jobs
  ret.first->second.theComputers.insert("comp1");

  myTableOut = EdgeControllerEtsiBalanced::table(myTableIn, myDesc);
  std::map<std::string, int> myCounters;
  myCounter = 0;
  for (const auto& myRow : myTableOut) {
    EXPECT_EQ("client" + std::to_string(myCounter), std::get<0>(myRow));
    EXPECT_EQ("lambda0", std::get<1>(myRow));
    myCounters[std::get<2>(myRow)]++;
    ++myCounter;
  }
  EXPECT_EQ(2u, myCounters.size());
  EXPECT_TRUE((myCounters["comp0"] == 2 and myCounters["comp1"] == 3) or
              (myCounters["comp0"] == 3 and myCounters["comp1"] == 2));

  // add a computer that can serve lambda1, check they split jobs
  myDesc.emplace("lambda1", DescMap::mapped_type{"comp2"});

  myTableOut = EdgeControllerEtsiBalanced::table(myTableIn, myDesc);
  myCounters.clear();
  myCounter = 0;
  for (const auto& myRow : myTableOut) {
    EXPECT_EQ("client" + std::to_string(myCounter), std::get<0>(myRow));
    EXPECT_EQ("lambda" + (myCounter < 5 ? std::string("0") : std::string("1")),
              std::get<1>(myRow));
    myCounters[std::get<2>(myRow)]++;
    ++myCounter;
  }
  EXPECT_EQ(3u, myCounters.size());
  EXPECT_EQ(5u, myCounters["comp0"] + myCounters["comp1"]);
  EXPECT_EQ(1u, myCounters["comp2"]);

  //
  // complex scenarios
  //
  // three computers:
  // comp0: lambda0, lambda1
  // comp1: lambda0, lambda1
  // comp2: lambda0, lambda1, lambda2
  //
  // lots of clients requesting all three lambdas
  //

  myDesc.clear();
  ret = myDesc.emplace("lambda0", DescMap::mapped_type{"comp0"});
  ret.first->second.theComputers.insert("comp1");
  ret = myDesc.emplace("lambda1", DescMap::mapped_type{"comp0"});
  ret.first->second.theComputers.insert("comp1");
  ret = myDesc.emplace("lambda2", DescMap::mapped_type{"comp0"});
  ret.first->second.theComputers.insert("comp1");
  ret.first->second.theComputers.insert("comp2");

  myTableIn.clear();
  for (auto i = 0; i < 100; i++) {
    for (auto j = 0; j < 3; j++) {
      myTableIn.emplace_back(
          "client" + std::to_string(i), "lambda" + std::to_string(j), "unused");
    }
  }

  myTableOut = EdgeControllerEtsiBalanced::table(myTableIn, myDesc);
  EXPECT_EQ(300u, myTableOut.size());

  std::map<std::string, int> myClientCounters;
  std::map<std::string, int> myLambdaCounters;
  myCounters.clear();
  for (const auto& myRow : myTableOut) {
    myClientCounters[std::get<0>(myRow)]++;
    myLambdaCounters[std::get<1>(myRow)]++;
    myCounters[std::get<2>(myRow)]++;
  }
  for (const auto& elem : myClientCounters) {
    EXPECT_EQ(3u, elem.second);
  }
  for (const auto& elem : myLambdaCounters) {
    EXPECT_EQ(100u, elem.second);
  }
  EXPECT_EQ(3u, myCounters.size());
  EXPECT_EQ(100u, myCounters["comp0"]);
  EXPECT_EQ(100u, myCounters["comp1"]);
  EXPECT_EQ(100u, myCounters["comp2"]);
}

} // namespace edge
} // namespace uiiit
