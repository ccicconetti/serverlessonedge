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

#include "Edge/Model/dag.h"
#include "Edge/Model/dagfactory.h"

#include "gtest/gtest.h"

#include <nlohmann/json.hpp>

#include <glog/logging.h>

namespace uiiit {
namespace edge {
namespace model {

struct TestDag : public ::testing::Test {};

TEST_F(TestDag, test_serialize_deserialize) {
  const auto myDag        = exampleDag();
  const auto mySerialized = myDag.toJson();
  VLOG(1) << '\n' << mySerialized;
  const auto myDeserialized = Dag::fromJson(mySerialized);
  ASSERT_EQ(myDag, myDeserialized);
}

TEST_F(TestDag, test_access_methods) {
  const auto myDag = exampleDag();
  VLOG(1) << myDag.toString();

  ASSERT_EQ(std::set<std::string>({"f0", "f1", "f2"}), myDag.uniqueFunctions());
  ASSERT_EQ(std::set<std::string>({"s0", "s1", "s2", "s3"}),
            myDag.states().allStates(true));
  ASSERT_EQ(std::set<std::string>({"s0", "s1", "s2"}),
            myDag.states().allStates(false));
  ASSERT_EQ(
      Dag::Dependencies(
          {{"s0", {"f0"}}, {"s1", {"f0", "f1"}}, {"s2", {"f2"}}, {"s3", {}}}),
      myDag.states().dependencies());
  ASSERT_EQ(std::set<std::string>(), myDag.states().states("fX"));
  ASSERT_EQ(std::set<std::string>({"s0", "s1"}), myDag.states().states("f0"));
  ASSERT_EQ(std::set<std::string>({"s1"}), myDag.states().states("f1"));
  ASSERT_EQ("f0-f1-f2-f2", myDag.name());

  ASSERT_EQ(std::set<std::string>({"f1", "f2"}), myDag.successorNames(0));
  ASSERT_EQ(std::set<std::string>({"f2"}), myDag.successorNames(1));
  ASSERT_EQ(std::set<std::string>({"f2"}), myDag.successorNames(2));
  ASSERT_EQ(std::set<std::string>({}), myDag.successorNames(3));
  ASSERT_THROW(myDag.successorNames(4), std::runtime_error);

  ASSERT_EQ(std::set<std::string>({}), myDag.predecessorNames(0));
  ASSERT_EQ(std::set<std::string>({"f0"}), myDag.predecessorNames(1));
  ASSERT_EQ(std::set<std::string>({"f0"}), myDag.predecessorNames(2));
  ASSERT_EQ(std::set<std::string>({"f1", "f2"}), myDag.predecessorNames(3));
  ASSERT_THROW(myDag.predecessorNames(4), std::runtime_error);
}

TEST_F(TestDag, test_invalid) {
  ASSERT_THROW(
      Dag({{1, 2}, {3}, {3}}, {"f0", "f1", "f2", "f3"}, {{"s", {"fX"}}}),
      std::runtime_error);
  ASSERT_THROW(Dag({{1, 2}, {3}, {3}}, {"f0", "f1", "f2"}, {{"s", {"f0"}}}),
               std::runtime_error);
  ASSERT_THROW(Dag({{1, 2}, {3}}, {"f0", "f1", "f2", "f3"}, {{"s", {"f0"}}}),
               std::runtime_error);
  ASSERT_NO_THROW(
      Dag({{1, 2}, {3}, {3}}, {"f0", "f1", "f2", "f3"}, {{"s", {"f0"}}}));

  ASSERT_THROW(Dag::fromJson("{\n"
                             "  \"dependencies\": {\n"
                             "    \"s0\": [\n"
                             "      \"f0\"\n"
                             "    ],\n"
                             "    \"s1\": [\n"
                             "      \"f0\",\n"
                             "      \"f1\"\n"
                             "    ],\n"
                             "    \"s2\": [\n"
                             "      \"f2\"\n"
                             "    ],\n"
                             "    \"s3\": null\n"
                             "  },\n"
                             "  \"functionNames\": [\n"
                             "    \"f0\",\n"
                             "    \"f1\",\n"
                             "    \"f2\",\n"
                             "    \"f3\"\n"
                             "  ]\n"
                             "}"),
               std::runtime_error);

  ASSERT_THROW(Dag::fromJson("{\n"
                             "  \"dependencies\": {\n"
                             "    \"s0\": [\n"
                             "      \"f0\"\n"
                             "    ],\n"
                             "    \"s1\": [\n"
                             "      \"f0\",\n"
                             "      \"f1\"\n"
                             "    ],\n"
                             "    \"s2\": [\n"
                             "      \"f2\"\n"
                             "    ],\n"
                             "    \"s3\": null\n"
                             "  },\n"
                             "  \"successors\": [\n"
                             "    [\n"
                             "      1,\n"
                             "      2\n"
                             "    ],\n"
                             "    [\n"
                             "      3\n"
                             "    ],\n"
                             "    [\n"
                             "      3\n"
                             "    ]\n"
                             "  ]\n"
                             "}"),
               std::runtime_error);

  ASSERT_THROW(Dag::fromJson("{\n"
                             "  \"functionNames\": [\n"
                             "    \"f0\",\n"
                             "    \"f1\",\n"
                             "    \"f2\",\n"
                             "    \"f3\"\n"
                             "  ],\n"
                             "  \"successors\": [\n"
                             "    [\n"
                             "      1,\n"
                             "      2\n"
                             "    ],\n"
                             "    [\n"
                             "      3\n"
                             "    ],\n"
                             "    [\n"
                             "      3\n"
                             "    ]\n"
                             "  ]\n"
                             "}"),
               std::runtime_error);

  ASSERT_THROW(Dag::fromJson("not-a-json"), nlohmann::detail::parse_error);
}

TEST_F(TestDag, test_factory_example) {
  ASSERT_NO_THROW(Dag::fromJson(DagFactory::exampleDag()));
}

} // namespace model
} // namespace edge
} // namespace uiiit
