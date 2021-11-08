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
#include "Edge/Model/dag.h"
#include "Edge/edgemessages.h"

#include "gtest/gtest.h"

#include <glog/logging.h>

namespace uiiit {
namespace edge {

struct TestEdgeMessages : public ::testing::Test {};

TEST_F(TestEdgeMessages, test_ctor) {
  ASSERT_NO_THROW(LambdaRequest("name", "input"));
  ASSERT_NO_THROW(LambdaRequest("name", "input", "datain"));
  ASSERT_NO_THROW(LambdaResponse("name", "output"));
  ASSERT_NO_THROW(LambdaResponse("name", "output", {0.1, 0.2, 0.3}));
}

TEST_F(TestEdgeMessages, test_request_serialize_deserialize) {
  LambdaRequest myRequest("name", "input", "datain");
  myRequest.states().emplace("state0", State::fromContent("content"));
  myRequest.states().emplace("state1", State::fromContent("another_content"));
  myRequest.states().emplace("state2", State::fromLocation("1.2.3.4:6666"));
  myRequest.theCallback = "1.2.3.4:6666";
  LOG(INFO) << myRequest.toString();

  const auto    myReqSerialized = myRequest.toProtobuf();
  LambdaRequest myReqDeserialized(myReqSerialized);
  ASSERT_TRUE(myRequest == myReqDeserialized)
      << "\n"
      << myRequest.toString() << "\nvs.\n"
      << myReqDeserialized.toString();
}

TEST_F(TestEdgeMessages, test_request_serialize_deserialize_chain) {
  LambdaRequest myRequest("", "input", "datain");
  myRequest.states().emplace("state0", State::fromContent("content"));
  myRequest.states().emplace("state1", State::fromContent("another_content"));
  myRequest.states().emplace("state2", State::fromLocation("1.2.3.4:6666"));
  myRequest.theCallback = "1.2.3.4:6666";
  myRequest.theChain    = std::make_unique<model::Chain>(model::exampleChain());
  LOG(INFO) << myRequest.toString();

  const auto    myReqSerialized = myRequest.toProtobuf();
  LambdaRequest myReqDeserialized(myReqSerialized);
  ASSERT_TRUE(myRequest == myReqDeserialized)
      << "\n"
      << myRequest.toString() << "\nvs.\n"
      << myReqDeserialized.toString();
}

TEST_F(TestEdgeMessages, test_request_serialize_deserialize_dag) {
  LambdaRequest myRequest("", "input", "datain");
  myRequest.states().emplace("state0", State::fromContent("content"));
  myRequest.states().emplace("state1", State::fromContent("another_content"));
  myRequest.states().emplace("state2", State::fromLocation("1.2.3.4:6666"));
  myRequest.theCallback = "1.2.3.4:6666";
  myRequest.theDag      = std::make_unique<model::Dag>(model::exampleDag());
  LOG(INFO) << myRequest.toString();

  const auto    myReqSerialized = myRequest.toProtobuf();
  LambdaRequest myReqDeserialized(myReqSerialized);
  ASSERT_TRUE(myRequest == myReqDeserialized)
      << "\n"
      << myRequest.toString() << "\nvs.\n"
      << myReqDeserialized.toString();
}

TEST_F(TestEdgeMessages, test_response_serialize_deserialize_sync) {
  LambdaResponse myResponse("name", "output", {0.1, 0.2, 0.3});
  myResponse.states().emplace("state0", State::fromContent("content"));
  myResponse.states().emplace("state1", State::fromContent("another_content"));
  myResponse.states().emplace("state2", State::fromLocation("1.2.3.4:6666"));
  LOG(INFO) << myResponse.toString();

  const auto     myResSerialized = myResponse.toProtobuf();
  LambdaResponse myResDeserialized(myResSerialized);
  ASSERT_TRUE(myResponse == myResDeserialized)
      << "\n"
      << myResponse.toString() << "\nvs.\n"
      << myResDeserialized.toString();
}

TEST_F(TestEdgeMessages, test_response_serialize_deserialize_async) {
  LambdaResponse myResponse;
  LOG(INFO) << myResponse.toString();

  const auto     myResSerialized = myResponse.toProtobuf();
  LambdaResponse myResDeserialized(myResSerialized);
  ASSERT_TRUE(myResponse == myResDeserialized)
      << "\n"
      << myResponse.toString() << "\nvs.\n"
      << myResDeserialized.toString();
}

} // namespace edge
} // namespace uiiit
