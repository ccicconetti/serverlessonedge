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

#include "Edge/edgemessages.h"
#include "Edge/lambda.h"
#include "Edge/processor.h"

#include "gtest/gtest.h"

namespace uiiit {
namespace edge {

struct TestLambda : public ::testing::Test {
  TestLambda()
      : theName("lambda1") {
  }
  const std::string theName;
};

TEST_F(TestLambda, test_ctor) {
  ASSERT_NO_THROW(Lambda(theName, [](const Processor&, const LambdaRequest) {
    return LambdaRequirements{0, 0};
  }));
}

TEST_F(TestLambda, test_proportional) {
  Lambda myLambda(theName, ProportionalRequirements(1, -100, 2, -200));

  Processor myProc("cpu1", ProcessorType::GenericCpu, 1000, 1, 42);

  {
    LambdaRequest myRequest(theName, std::string(1000, 'x'));
    const auto    myReq = myLambda.requirements(myProc, myRequest);
    ASSERT_EQ(900u, myReq.theOperations);
    ASSERT_EQ(1800u, myReq.theMemory);
  }
  {
    LambdaRequest myRequest(theName, std::string(100, 'x'));
    const auto    myReq = myLambda.requirements(myProc, myRequest);
    ASSERT_EQ(0u, myReq.theOperations);
    ASSERT_EQ(0u, myReq.theMemory);
  }
}

TEST_F(TestLambda, test_fixed) {
  Lambda myLambda(theName, FixedRequirements(1, 2));

  Processor myProc("cpu1", ProcessorType::GenericCpu, 1000, 1, 42);

  LambdaRequest myRequest(theName, std::string(1000, 'x'));
  const auto    myReq = myLambda.requirements(myProc, myRequest);
  ASSERT_EQ(1u, myReq.theOperations);
  ASSERT_EQ(2u, myReq.theMemory);
}

TEST_F(TestLambda, test_exec_copy_input) {
  Lambda myLambda(theName, FixedRequirements(1, 2));

  LambdaRequest         myRequest(theName, "batman");
  std::array<double, 3> myLoads({0.2, 0.5, 0.1});
  const auto            myResponse = myLambda.execute(myRequest, myLoads);

  EXPECT_EQ(20, myResponse->theLoad1);
  EXPECT_EQ(50, myResponse->theLoad10);
  EXPECT_EQ(10, myResponse->theLoad30);

  EXPECT_EQ("batman", myResponse->theOutput);

  ASSERT_EQ("OK", myResponse->theRetCode);
}


TEST_F(TestLambda, test_exec_fixed_output) {
  Lambda myLambda(theName, "robin", FixedRequirements(1, 2));

  LambdaRequest         myRequest(theName, "batman");
  std::array<double, 3> myLoads({0.2, 0.5, 0.1});
  const auto            myResponse = myLambda.execute(myRequest, myLoads);

  EXPECT_EQ(20, myResponse->theLoad1);
  EXPECT_EQ(50, myResponse->theLoad10);
  EXPECT_EQ(10, myResponse->theLoad30);

  EXPECT_EQ("robin", myResponse->theOutput);

  ASSERT_EQ("OK", myResponse->theRetCode);
}

} // namespace edge
} // namespace uiiit
