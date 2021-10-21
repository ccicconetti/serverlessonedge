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

#include "gtest/gtest.h"

#include <nlohmann/json.hpp>

#include <glog/logging.h>

namespace uiiit {
namespace edge {
namespace model {

struct TestChain : public ::testing::Test {};

TEST_F(TestChain, test_serialize_deserialize) {
  Chain      myChain({"f1", "f2"},
                {
                    {
                        "s0",
                        {"f1"},
                    },
                    {
                        "s1",
                        {"f1", "f2"},
                    },
                    {
                        "s2",
                        {"f2"},
                    },
                    {
                        "s3",
                        {},
                    },
                });
  const auto mySerialized = myChain.toJson();
  VLOG(1) << '\n' << mySerialized;
  const auto myDeserialized = Chain::fromJson(mySerialized);
  ASSERT_EQ(myChain, myDeserialized);
}

TEST_F(TestChain, test_invalid) {
  ASSERT_THROW(Chain({"f1", "f2"}, {{"s", {"fX"}}}), std::runtime_error);

  std::string myJson = "{"
                       "  \"dependencies\": {"
                       "    \"s\": ["
                       "      \"fX\""
                       "    ]"
                       "  },"
                       "  \"functions\": ["
                       "    \"f1\","
                       "    \"f2\""
                       "  ]"
                       "}";

  ASSERT_THROW(Chain::fromJson(myJson), std::runtime_error);

  ASSERT_THROW(Chain::fromJson("not-a-json"), nlohmann::detail::parse_error);
}

} // namespace model
} // namespace edge
} // namespace uiiit
