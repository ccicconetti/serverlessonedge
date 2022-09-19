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
#include "Edge/edgeserver.h"
#include "Edge/edgeservergrpc.h"

#include "gtest/gtest.h"

#include <cctype>
#include <chrono>
#include <stdexcept>
#include <thread>

namespace uiiit {
namespace edge {

class TrivialEdgeServer final : public EdgeServer
{
 public:
  explicit TrivialEdgeServer(const std::string& aEndpoint,
                             const std::string& aLambda,
                             const std::string& aThrowingLambda)
      : EdgeServer(aEndpoint)
      , theLambda(aLambda)
      , theThrowingLambda(aThrowingLambda) {
    // noop
  }

  // convert input to uppercase
  // throw if the lambda's name does not match
  rpc::LambdaResponse process(const rpc::LambdaRequest& aReq) override {
    rpc::LambdaResponse ret;

    if (aReq.name() == theThrowingLambda) {
      throw std::runtime_error("exception raised");
    } else if (aReq.name() != theLambda) {
      ret.set_retcode("invalid lambda: " + aReq.name());

    } else {
      ret.set_retcode("OK");
      std::string myResponse(aReq.input().size(), '\0');
      for (size_t i = 0; i < aReq.input().size(); i++) {
        myResponse[i] = std::toupper(aReq.input()[i]);
      }
      ret.set_output(myResponse);
    }
    return ret;
  }

 private:
  const std::string theLambda;
  const std::string theThrowingLambda;
};

struct TestEdgeServerGrpc : public ::testing::Test {
  TestEdgeServerGrpc()
      : theEndpoint("localhost:6666") {
  }
  void              runLambda();
  const std::string theEndpoint;
};

TEST_F(TestEdgeServerGrpc, test_invalid_endpoint) {
  {
    TrivialEdgeServer myEdgeServer("localhost:100", "my-lambda", "my-bomb");
    EdgeServerGrpc    myEdgeServerGrpc(myEdgeServer, 1);
    ASSERT_THROW(myEdgeServerGrpc.run(), std::runtime_error);
  }
  {
    TrivialEdgeServer myEdgeServer("1.2.3.4:10000", "my-lambda", "my-bomb");
    EdgeServerGrpc    myEdgeServerGrpc(myEdgeServer, 1);
    ASSERT_THROW(myEdgeServerGrpc.run(), std::runtime_error);
  }
}

void TestEdgeServerGrpc::runLambda() {
  // create the server
  TrivialEdgeServer myEdgeServer(theEndpoint, "my-lambda", "my-bomb");
  EdgeServerGrpc    myEdgeServerGrpc(myEdgeServer, 1);
  myEdgeServerGrpc.run();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // create the client and run a lambda
  EdgeClientGrpc myClient(theEndpoint);
  LambdaRequest  myReq("my-lambda", "Hello world!");
  const auto     myRes(myClient.RunLambda(myReq, false));
  ASSERT_EQ("OK", myRes.theRetCode);
  ASSERT_EQ("HELLO WORLD!", myRes.theOutput);

  // run an unsupported lambda
  LambdaRequest myInvalidReq("not-my-lambda", "Hello world!");
  const auto    myInvalidRes(myClient.RunLambda(myInvalidReq, false));
  ASSERT_EQ("invalid lambda: not-my-lambda", myInvalidRes.theRetCode);
  ASSERT_EQ("", myInvalidRes.theOutput);

  // run a lambda that causes the server to raise an exception
  LambdaRequest myBombReq("my-bomb", "Hello world!");
  const auto    myBombRes(myClient.RunLambda(myBombReq, false));
  ASSERT_EQ("invalid 'my-bomb' request: exception raised",
            myBombRes.theRetCode);
  ASSERT_EQ("", myBombRes.theOutput);
}

TEST_F(TestEdgeServerGrpc, test_run_lambda_insecure) {
  runLambda();
}

} // namespace edge
} // namespace uiiit
