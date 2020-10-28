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

#include "Edge/forwardingtable.h"
#include "Edge/forwardingtableexceptions.h"
#include "Support/chrono.h"
#include "Support/tostring.h"
#include "Support/wait.h"

#include "gtest/gtest.h"

#include <cmath>

#include <glog/logging.h>

namespace uiiit {
namespace edge {

struct TestForwardingTable : public ::testing::Test {};

TEST_F(TestForwardingTable, test_ctor) {
  ASSERT_NO_THROW((ForwardingTable(ForwardingTable::Type::Random)));
  ASSERT_NO_THROW((ForwardingTable(ForwardingTable::Type::LeastImpedance)));
}

TEST_F(TestForwardingTable, test_pf_ctor) {
  ASSERT_NO_THROW(
      (ForwardingTable(ForwardingTable::Type::ProportionalFairness)));
  LOG(INFO) << ">> RICHIAMATO COSTRUTTORE FORWARDING_TABLE(pf)";
}

TEST_F(TestForwardingTable, test_pf_operations) {
  ForwardingTable myTable(ForwardingTable::Type::ProportionalFairness);

  myTable.change("lambda1", "dest1:666", 1, true);
  myTable.change("lambda1", "dest2:666", 0.5, false);
  myTable.change("lambda1", "dest2:667", 99, true);

  myTable.change("lambda2", "dest1:666", 1, true);
  myTable.change("lambda2", "dest2:666", 0.5, false);

  myTable.change("another_lambda", "dest3:666", 42, false);

  ASSERT_EQ(std::string("another_lambda [42 ] dest3:666\n"
                        "lambda1        [1  ] dest1:666 (F)\n"
                        "               [0.5] dest2:666\n"
                        "               [99 ] dest2:667 (F)\n"
                        "lambda2        [1  ] dest1:666 (F)\n"
                        "               [0.5] dest2:666\n"),
            ::toString(myTable));

  std::string ret = myTable("lambda1");
  LOG(INFO) << "Destinazione per \"Lambda1\" = " << ret;
  ASSERT_EQ(ret, std::string("dest2:667"));

  ret = myTable("lambda2");
  LOG(INFO) << "Destinazione per \"Lambda2\" = " << ret;
  ASSERT_EQ(ret, std::string("dest1:666"));

  myTable.remove("lambda1", "dest2:667");
  ret = myTable("lambda1");
  LOG(INFO) << "Destinazione per \"Lambda1\" DOPO la rimozione = " << ret;
  ASSERT_EQ(ret, std::string("dest1:666"));
}

TEST_F(TestForwardingTable, test_add_remove) {
  ForwardingTable myTable(ForwardingTable::Type::Random);

  myTable.change("lambda1", "dest1:666", 1, true);
  myTable.change("lambda1", "dest2:666", 0.5, false);
  myTable.change("lambda1", "dest2:667", 99, true);

  myTable.change("lambda2", "dest1:666", 1, true);
  myTable.change("lambda2", "dest2:666", 0.5, false);

  myTable.change("another_lambda", "dest3:666", 42, false);

  ASSERT_EQ(std::string("another_lambda [42 ] dest3:666\n"
                        "lambda1        [1  ] dest1:666 (F)\n"
                        "               [0.5] dest2:666\n"
                        "               [99 ] dest2:667 (F)\n"
                        "lambda2        [1  ] dest1:666 (F)\n"
                        "               [0.5] dest2:666\n"),
            ::toString(myTable));

  myTable.remove("lambda1", "dest2:666");

  ASSERT_EQ(std::string("another_lambda [42 ] dest3:666\n"
                        "lambda1        [1  ] dest1:666 (F)\n"
                        "               [99 ] dest2:667 (F)\n"
                        "lambda2        [1  ] dest1:666 (F)\n"
                        "               [0.5] dest2:666\n"),
            ::toString(myTable));

  myTable.change("lambda1", "dest2:667", 98);

  ASSERT_EQ(std::string("another_lambda [42 ] dest3:666\n"
                        "lambda1        [1  ] dest1:666 (F)\n"
                        "               [98 ] dest2:667 (F)\n"
                        "lambda2        [1  ] dest1:666 (F)\n"
                        "               [0.5] dest2:666\n"),
            ::toString(myTable));

  myTable.multiply("lambda1", "dest2:667", 0.5);

  ASSERT_EQ(std::string("another_lambda [42 ] dest3:666\n"
                        "lambda1        [1  ] dest1:666 (F)\n"
                        "               [49 ] dest2:667 (F)\n"
                        "lambda2        [1  ] dest1:666 (F)\n"
                        "               [0.5] dest2:666\n"),
            ::toString(myTable));

  myTable.remove("lambda1", "dest1:666");
  myTable.remove("lambda1", "dest2:667");

  ASSERT_EQ(std::string("another_lambda [42 ] dest3:666\n"
                        "lambda2        [1  ] dest1:666 (F)\n"
                        "               [0.5] dest2:666\n"),
            ::toString(myTable));

  myTable.remove("lambda2");

  ASSERT_EQ(std::string("another_lambda [42] dest3:666\n"),
            ::toString(myTable));

  myTable.remove("another_lambda");

  ASSERT_EQ(std::string(""), ::toString(myTable));
}

TEST_F(TestForwardingTable, test_invalid_operations) {
  ForwardingTable myTable(ForwardingTable::Type::Random);

  ASSERT_THROW(myTable.change("lambda1", "", 1), NoDestinations);
  ASSERT_THROW(myTable.change("lambda1", "", 1, true), InvalidDestination);
  ASSERT_THROW(myTable.change("lambda1", "valid", 0), InvalidDestination);
  ASSERT_NO_THROW(myTable.change("lambda1", "valid", 99, true));
  ASSERT_NO_THROW(myTable.change("lambda1", "valid", 49));
  ASSERT_THROW(myTable.change("lambda1", "valid", -99), InvalidWeight);
  ASSERT_NO_THROW(myTable.multiply("lambda1", "valid", 99));
  ASSERT_THROW(myTable.multiply("lambda1", "valid", -99), InvalidWeightFactor);

  ASSERT_NO_THROW(myTable("lambda1"));

  ASSERT_NO_THROW(myTable.remove("lambda1"));
  ASSERT_NO_THROW(myTable.remove("lambda1"));

  ASSERT_THROW(myTable("lambda1"), NoDestinations);
}

TEST_F(TestForwardingTable, test_access_random) {
  ForwardingTable myTable(ForwardingTable::Type::Random);

  myTable.change("lambda1", "dest1", 1, true);
  myTable.change("lambda1", "dest2", 1.0 / 3, true);
  myTable.change("lambda1", "dest3", 1.0 / 6, true);

  const size_t                 myRuns = 10000;
  std::map<std::string, float> myCounters;
  for (size_t i = 0; i < myRuns; i++) {
    myCounters[myTable("lambda1")]++;
  }

  ASSERT_EQ(3, int(round(myCounters["dest2"] / myCounters["dest1"])));
  ASSERT_EQ(2, int(round(myCounters["dest3"] / myCounters["dest2"])));
}

TEST_F(TestForwardingTable, test_access_least_impedance) {
  ForwardingTable myTable(ForwardingTable::Type::LeastImpedance);

  myTable.change("lambda1", "dest1", 6, true);
  myTable.change("lambda1", "dest2", 3, true);
  myTable.change("lambda1", "dest3", 1, true);

  ASSERT_EQ("dest3", myTable("lambda1"));

  myTable.remove("lambda1", "dest3");
  ASSERT_EQ("dest2", myTable("lambda1"));

  myTable.remove("lambda1", "dest2");
  ASSERT_EQ("dest1", myTable("lambda1"));
}

TEST_F(TestForwardingTable, test_access_round_robin) {
  ForwardingTable myTable(ForwardingTable::Type::RoundRobin);

  std::map<std::string, double> myLatencies(
      {{"dest1", 100}, {"dest2", 110}, {"dest3", 1000}});

  for (const auto& myPair : myLatencies) {
    myTable.change("lambda1", myPair.first, 1, true);
  }

  std::list<std::string> myDestinations;

  for (auto i = 0; i < 10; i++) {
    const auto myDest = myTable("lambda1");
    myDestinations.push_back(myDest);
    myTable.change("lambda1", myDest, myLatencies[myDest]);
  }

  ASSERT_EQ(std::list<std::string>({"dest1",
                                    "dest2",
                                    "dest3",
                                    "dest1",
                                    "dest2",
                                    "dest1",
                                    "dest2",
                                    "dest1",
                                    "dest2",
                                    "dest1"}),
            myDestinations);

  myLatencies["dest1"] = 400;

  myDestinations.clear();
  for (auto i = 0; i < 10; i++) {
    const auto myDest = myTable("lambda1");
    myDestinations.push_back(myDest);
    myTable.change("lambda1", myDest, myLatencies[myDest]);
  }

  ASSERT_EQ(std::list<std::string>({"dest2",
                                    "dest1",
                                    "dest2",
                                    "dest2",
                                    "dest2",
                                    "dest2",
                                    "dest2",
                                    "dest2",
                                    "dest2",
                                    "dest2"}),
            myDestinations);

  myTable.remove("lambda1", "dest2");

  myDestinations.clear();
  for (auto i = 0; i < 5; i++) {
    const auto myDest = myTable("lambda1");
    myDestinations.push_back(myDest);
    myTable.change("lambda1", myDest, myLatencies[myDest]);
  }

  ASSERT_EQ(
      std::list<std::string>({"dest1", "dest1", "dest1", "dest1", "dest1"}),
      myDestinations);

  myTable.remove("lambda1", "dest1");

  myDestinations.clear();
  for (auto i = 0; i < 5; i++) {
    const auto myDest = myTable("lambda1");
    myDestinations.push_back(myDest);
    myTable.change("lambda1", myDest, myLatencies[myDest]);
  }

  ASSERT_EQ(
      std::list<std::string>({"dest3", "dest3", "dest3", "dest3", "dest3"}),
      myDestinations);

  myTable.change("lambda1", "dest1", myLatencies["dest1"], true);

  myDestinations.clear();
  for (auto i = 0; i < 5; i++) {
    const auto myDest = myTable("lambda1");
    myDestinations.push_back(myDest);
    myTable.change("lambda1", myDest, myLatencies[myDest], true);
  }

  ASSERT_EQ(
      std::list<std::string>({"dest1", "dest1", "dest1", "dest1", "dest1"}),
      myDestinations);
}

TEST_F(TestForwardingTable, DISABLED_test_access_round_robin_stale) {
  ForwardingTable myTable(ForwardingTable::Type::RoundRobin);

  myTable.change("lambda1", "dest1", 1, true);
  myTable.change("lambda1", "dest2", 1, true);

  ASSERT_EQ("dest1", myTable("lambda1"));
  myTable.change("lambda1", "dest1", 100);
  ASSERT_EQ("dest2", myTable("lambda1"));
  myTable.change("lambda1", "dest2", 100);
  ASSERT_EQ("dest1", myTable("lambda1"));
  ASSERT_EQ("dest2", myTable("lambda1"));

  support::Chrono myChrono(true);
  support::waitFor<std::string>(
      [&myTable] {
        const auto myDest = myTable("lambda1");
        myTable.change("lambda1", "dest1", 10);
        return myDest;
      },
      "dest2",
      11);
  ASSERT_EQ(10, static_cast<int>(myChrono.stop()));

  ASSERT_EQ("dest1", myTable("lambda1"));
  ASSERT_EQ("dest1", myTable("lambda1"));
}

} // namespace edge
} // namespace uiiit
