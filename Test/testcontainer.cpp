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

#include "Edge/container.h"
#include "Edge/edgemessages.h"
#include "Edge/lambda.h"
#include "Edge/processor.h"

#include "gtest/gtest.h"

#include <glog/logging.h> // XXX

namespace uiiit {
namespace edge {

struct TestContainer : public ::testing::Test {
  // each task requires 1 operation and 1 byte
  // the processor has memory with 10 bytes
  TestContainer()
      : theName("lambda1")
      , theLambda(theName, FixedRequirements(1, 1))
      , theSpeed(42.0)
      , theProcessor("cpu1", ProcessorType::GenericCpu, theSpeed, 1, 10) {
  }
  const std::string theName;
  const Lambda      theLambda;
  const double      theSpeed;
  Processor         theProcessor;
};

TEST_F(TestContainer, test_ctor) {
  ASSERT_NO_THROW((Container{"container1", theProcessor, theLambda, 10}));
}

TEST_F(TestContainer, test_scheduling_worker_bound) {
  LambdaRequest myReq(theName, "pippo");

  Container myContainer("container1", theProcessor, theLambda, 5);

  for (size_t i = 0; i < 10; i++) {
    myContainer.push(myReq, i);
    ASSERT_FLOAT_EQ(1.0 / (theSpeed / myContainer.active()),
                    myContainer.nearest());
    if (i < 5) {
      ASSERT_EQ(i + 1, myContainer.active());
      ASSERT_EQ(0, myContainer.pending());
    } else {
      ASSERT_EQ(5, myContainer.active());
      ASSERT_EQ(i - 4, myContainer.pending());
    }
  }

  for (size_t i = 0; i < 10; i++) {
    const auto myTask = myContainer.pop();
    ASSERT_EQ(i, myTask.theId);
    ASSERT_EQ(1u, myTask.theMemory);
    ASSERT_EQ((i % 5 == 0) ? 1u : 0u, myTask.theResidualOps);
    ASSERT_TRUE(static_cast<bool>(myTask.theResp));
    ASSERT_EQ("pippo", myTask.theResp->theOutput);
  }
}

TEST_F(TestContainer, test_simulation) {
  LambdaRequest myReq(theName, "pippo");

  Container myContainer("container1", theProcessor, theLambda, 5);

  ASSERT_FLOAT_EQ(1.0 / theSpeed, myContainer.simulate(myReq));

  for (size_t i = 0; i < 10; i++) {
    myContainer.push(myReq, i);
    ASSERT_FLOAT_EQ(1.0 / (theSpeed / myContainer.active()),
                    myContainer.nearest());
    if (i < 4) {
      ASSERT_FLOAT_EQ((2 + i) / theSpeed, myContainer.simulate(myReq)) << i;
    } else {
      ASSERT_FLOAT_EQ(5 * (i - 2) / theSpeed, myContainer.simulate(myReq)) << i;
    }

    if (i < 5) {
      ASSERT_EQ(i + 1, myContainer.active());
      ASSERT_EQ(0, myContainer.pending());
    } else {
      ASSERT_EQ(5, myContainer.active());
      ASSERT_EQ(i - 4, myContainer.pending());
    }
  }

  ASSERT_FLOAT_EQ(35.0 / theSpeed, myContainer.simulate(myReq));
}

TEST_F(TestContainer, test_scheduling_memory_bound) {
  Processor myProcessor(
      "cpu1", ProcessorType::GenericCpu, 42, 1, 100); // 100 bytes
  Lambda myLambda(
      theName,
      ProportionalRequirements(10, 0, 10, 0)); // 10 bytes/ops every char
  LambdaRequest myReq10(theName, "a");
  LambdaRequest myReq20(theName, "ab");
  LambdaRequest myReq30(theName, "abc");

  Container myContainer("container1", myProcessor, myLambda,
                        10); // 10 workers

  struct OperationsToTime {
    OperationsToTime(Container& aContainer)
        : theContainer(aContainer) {
    }
    double operator()(const uint64_t aOperations) const noexcept {
      return aOperations /
             (theContainer.processor().speed() / theContainer.active());
    }
    Container& theContainer;
  };

  OperationsToTime myOtt(myContainer);

  // advance an empty container does not have effect
  ASSERT_NO_THROW(myContainer.advance(0));
  ASSERT_NO_THROW(myContainer.advance(1));

  // but cannot advance in the past
  ASSERT_THROW(myContainer.advance(-1), std::runtime_error);

  // 10 + 20 + 30x22 = 90 <= 100 : all tasks become active
  myContainer.push(myReq10, 0);
  myContainer.push(myReq20, 1);
  myContainer.push(myReq30, 2);
  myContainer.push(myReq30, 3);
  ASSERT_EQ(4u, myContainer.active());
  ASSERT_EQ(0u, myContainer.pending());

  // 90 + 30 > 100  : new task is pending
  myContainer.push(myReq30, 4);
  ASSERT_EQ(4u, myContainer.active());
  ASSERT_EQ(1u, myContainer.pending());

  // however a smaller request fits
  myContainer.push(myReq10, 5);
  ASSERT_EQ(5u, myContainer.active());
  ASSERT_EQ(1u, myContainer.pending());

  // a further smaller request does fit (full memory)
  myContainer.push(myReq10, 5);
  ASSERT_EQ(5u, myContainer.active());
  ASSERT_EQ(2u, myContainer.pending());

  // advance time by executing the first task
  // since the oldest pending task does not fit
  // in memory, no task is executed even though
  // there is a waiting worker
  ASSERT_FLOAT_EQ(myOtt(10), myContainer.nearest());
  ASSERT_EQ(0u, myContainer.pop().theId);
  ASSERT_EQ(4u, myContainer.active());
  ASSERT_EQ(2u, myContainer.pending());
  ASSERT_EQ(10u, myProcessor.memAvailable());

  // pop another task, still memory is not enough
  ASSERT_FLOAT_EQ(0, myContainer.nearest());
  ASSERT_EQ(5u, myContainer.pop().theId);
  ASSERT_EQ(3u, myContainer.active());
  ASSERT_EQ(2u, myContainer.pending());
  ASSERT_EQ(20u, myProcessor.memAvailable());

  // advance time by 5 operations
  ASSERT_FLOAT_EQ(myOtt(10), myContainer.nearest());
  myContainer.advance(myOtt(5));
  ASSERT_FLOAT_EQ(myOtt(5), myContainer.nearest());

  // advance again
  myContainer.advance(myOtt(5));
  ASSERT_EQ(0u, myContainer.nearest());

  // pop another task, now there is room for all pending tasks
  ASSERT_EQ(1u, myContainer.pop().theId);
  ASSERT_EQ(4u, myContainer.active());
  ASSERT_EQ(0u, myContainer.pending());
  ASSERT_EQ(0u, myProcessor.memAvailable());
}

} // namespace edge
} // namespace uiiit
