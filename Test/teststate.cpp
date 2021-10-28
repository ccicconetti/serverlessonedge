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

#include "Edge/edgemessages.h"
#include "Edge/stateclient.h"
#include "Edge/stateserver.h"

#include "gtest/gtest.h"

#include <glog/logging.h>

namespace uiiit {
namespace edge {

struct TestState : public ::testing::Test {};

TEST_F(TestState, test_client_server) {
  const std::string myEndpoint = "127.0.0.1:6480";
  StateServer       myServer(myEndpoint);
  myServer.run(false);
  StateClient myClient(myEndpoint);

  // write a few states
  for (size_t i = 0; i < 5; i++) {
    ASSERT_NO_THROW(
        myClient.Put("s" + std::to_string(i), "content-s" + std::to_string(i)));
  }

  // read a valid state
  std::string myContent;
  for (size_t i = 0; i < 5; i++) {
    ASSERT_TRUE(myClient.Get("s" + std::to_string(i), myContent));
    ASSERT_EQ("content-s" + std::to_string(i), myContent);
  }

  // overwrite content
  ASSERT_NO_THROW(myClient.Put("s0", "another-content-s0"));
  ASSERT_TRUE(myClient.Get("s0", myContent));
  ASSERT_EQ("another-content-s0", myContent);

  // read invalid states
  ASSERT_FALSE(myClient.Get("", myContent));
  ASSERT_FALSE(myClient.Get("sX", myContent));

  // delete an invalid state
  ASSERT_FALSE(myClient.Del(""));
  ASSERT_FALSE(myClient.Del("sX"));

  // delete a valid state
  ASSERT_TRUE(myClient.Del("s0"));
  EXPECT_FALSE(myClient.Get("s0", myContent));
  ASSERT_FALSE(myClient.Del("s0"));

  // write it again
  ASSERT_NO_THROW(myClient.Put("s0", "new-content-s0"));
  ASSERT_TRUE(myClient.Get("s0", myContent));
  ASSERT_EQ("new-content-s0", myContent);
}

} // namespace edge
} // namespace uiiit
