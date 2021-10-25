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

#include "Edge/callbackclient.h"
#include "Edge/callbackserver.h"
#include "Edge/edgemessages.h"
#include "Support/queue.h"

#include "gtest/gtest.h"

#include <glog/logging.h>

namespace uiiit {
namespace edge {

struct TestCallback : public ::testing::Test {};

TEST_F(TestCallback, test_client_server) {
  CallbackServer::Queue myQueue;
  const std::string     myEndpoint = "127.0.0.1:6480";
  CallbackServer        myServer(myEndpoint, myQueue);
  myServer.run(false);
  CallbackClient myClient(myEndpoint);

  LambdaResponse myResponse("OK", "output");
  for (auto i = 0; i < 5; i++) {
    ASSERT_NO_THROW(myClient.ReceiveResponse(myResponse));
  }
  ASSERT_NO_THROW(myClient.ReceiveResponse(LambdaResponse("ERR", "")));

  size_t myReceived = 0;
  while (true) {
    const auto myResponse = myQueue.pop();
    if (myResponse.theRetCode == "ERR") {
      break;
    }
    myReceived++;
  }
  ASSERT_EQ(5, myReceived);
}

} // namespace edge
} // namespace uiiit
