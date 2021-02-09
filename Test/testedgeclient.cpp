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

#include "Edge/edgeclientgrpc.h"

#include "gtest/gtest.h"

namespace uiiit {
namespace edge {

struct TestEdgeClient : public ::testing::Test {
  TestEdgeClient()
      : theEndpoint("localhost:666") {
  }
  const std::string theEndpoint;
};

TEST_F(TestEdgeClient, test_ctor) {
  ASSERT_NO_THROW(EdgeClientGrpc{theEndpoint});
}

TEST_F(TestEdgeClient, test_no_server) {
  EdgeClientGrpc myClient(theEndpoint);
  LambdaRequest  myReq("lambda1", "");
  ASSERT_THROW(myClient.RunLambda(myReq, true), std::runtime_error);
  ASSERT_THROW(myClient.RunLambda(myReq, false), std::runtime_error);
}

} // namespace edge
} // namespace uiiit
