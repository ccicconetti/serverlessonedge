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

#include "gtest/gtest.h"

#include "Edge/edgeclientgrpc.h"
#include "Edge/edgecomputerhttp.h"
#include "Edge/edgeservergrpc.h"
#include "Rest/server.h"

namespace uiiit {
namespace edge {

struct TrivialFaasPlatform : public rest::Server {
  TrivialFaasPlatform(const std::string& aUri)
      : rest::Server(aUri) {
    (*this)(web::http::methods::POST,
            "/function/clambda0",
            [this](web::http::http_request aReq) { handlePost(aReq); });
  }

  void handlePost(web::http::http_request aReq) {
    const auto myRequest = aReq.extract_json().get();
    if (not myRequest.has_field("input")) {
      aReq.reply(web::http::status_codes::BadRequest);
    } else {
      web::json::value myResponse;
      const auto       myInput = myRequest.at("input").as_string();
      myResponse["output"]     = web::json::value(myInput + myInput);
      aReq.reply(web::http::status_codes::OK, myResponse);
    }
  }
};

struct TestEdgeComputerHttp : public ::testing::Test {
  TestEdgeComputerHttp()
      : theGatewayUrl("http://localhost:10000/")
      , theEndpoint("localhost:10001") {
    // noop
  }
  const std::string theGatewayUrl;
  const std::string theEndpoint;
  const bool        theSecure = false;
};

TEST_F(TestEdgeComputerHttp, test_sync_server) {
  EdgeComputerHttp myEdgeComputerHttp(1,
                                      theEndpoint,
                                      theSecure,
                                      EdgeComputerHttp::Type::OPENFAAS_0_8,
                                      theGatewayUrl);
  EdgeServerGrpc   myServerGrpc(myEdgeComputerHttp, 1, theSecure);
  myServerGrpc.run();

  EdgeClientGrpc myClient(theEndpoint, theSecure);
  LambdaRequest  myReq("clambda0", "{\"input\" : \"abc\"}");

  // FaaS platform not started
  const auto myResponseFail = myClient.RunLambda(myReq, false);
  LOG(INFO) << myResponseFail;
  ASSERT_NE("OK", myResponseFail.theRetCode);
  ASSERT_EQ("", myResponseFail.theOutput);

  // start FaaS platform
  TrivialFaasPlatform myFaasPlatform(theGatewayUrl);
  myFaasPlatform.start();
  const auto myResponseSucc = myClient.RunLambda(myReq, false);
  LOG(INFO) << myResponseSucc;
  ASSERT_EQ("OK", myResponseSucc.theRetCode);
  ASSERT_EQ("{\"output\":\"abcabc\"}", myResponseSucc.theOutput);
}

TEST_F(TestEdgeComputerHttp, DISABLED_trivial_faas_platform) {
  TrivialFaasPlatform myFaasPlatform(theGatewayUrl);
  myFaasPlatform.start();
  ::pause();
}

} // namespace edge
} // namespace uiiit
