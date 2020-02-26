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

#include "Edge/computer.h"
#include "Edge/container.h"
#include "Edge/edgemessages.h"
#include "Edge/lambda.h"
#include "Edge/processor.h"
#include "Support/testutils.h"
#include "Support/tostring.h"

#include "gtest/gtest.h"

#include <glog/logging.h>

namespace uiiit {
namespace edge {

struct TestComputer : public ::testing::Test {
  // each task requires 1 operation and 1 byte
  // the processor has memory with 10 bytes
  TestComputer()
      : theName("computer1")
      , theLambda("lambda1", FixedRequirements(1, 1)) {
  }
  const std::string theName;
  const Lambda      theLambda;

  using RespPtr = std::shared_ptr<const LambdaResponse>;
  struct Collector {
    Collector(std::list<std::pair<uint64_t, RespPtr>>& aList)
        : theList(aList) {
    }
    void operator()(const uint64_t aId, const RespPtr& aResp) {
      theList.emplace_back(std::make_pair(aId, aResp));
    }
    std::list<std::pair<uint64_t, RespPtr>>& theList;
  };

  void single_container(const size_t aNumWorkers) {
    std::list<std::pair<uint64_t, RespPtr>> myList;
    Collector                               myCollector(myList);
    Computer myComputer(theName, myCollector, Computer::UtilCallback());

    myComputer.addProcessor("cpu", ProcessorType::GenericCpu, 100, 1, 1000);
    myComputer.addContainer("container",
                            "cpu",
                            Lambda("lambda", FixedRequirements(10, 1)),
                            aNumWorkers);

    LambdaRequest myReq("lambda", "input");

    support::Chrono myChrono(true);
    const auto      myId = myComputer.addTask(myReq);

    WAIT_FOR([&]() { return myList.size() == 1; }, 1.0);

    ASSERT_LT(0.1, myChrono.stop());

    ASSERT_EQ(myId, myList.front().first);
    ASSERT_TRUE(static_cast<bool>(myList.front().second));
    ASSERT_EQ("input", myList.front().second->theOutput);

    // add 10 tasks
    myChrono.start();
    for (auto i = 0; i < 10; i++) {
      ASSERT_EQ(i + 1, myComputer.addTask(myReq));
    }

    WAIT_FOR([&]() { return myList.size() == 11; }, 5.0);

    ASSERT_LT(1, myChrono.stop());
  }
};

TEST_F(TestComputer, test_ctor) {
  ASSERT_NO_THROW((Computer{
      theName,
      [](const uint64_t, const std::shared_ptr<const LambdaResponse>&) {},
      Computer::UtilCallback()}));
  ASSERT_THROW(
      (Computer{theName, Computer::Callback(), Computer::UtilCallback()}),
      std::runtime_error);
}

TEST_F(TestComputer, test_add_processors_containers) {
  Computer myComputer(
      theName,
      [](const uint64_t, const std::shared_ptr<const LambdaResponse>&) {},
      Computer::UtilCallback());

  for (char i = '1'; i < '4'; i++) {
    myComputer.addProcessor(
        std::string("cpu") + i, ProcessorType::GenericCpu, 100, 1, 1000);
  }

  for (char i = '1'; i < '4'; i++) {
    myComputer.addContainer(
        std::string("a_container") + i,
        std::string("cpu") + i,
        Lambda(std::string("a_lambda") + i, FixedRequirements(1, 1)),
        1 + i - '1');
    myComputer.addContainer(
        std::string("b_container") + i,
        std::string("cpu") + i,
        Lambda(std::string("b_lambda") + i, FixedRequirements(1, 1)),
        1 + i - '1');
  }

  // cannot add a processor with same name
  ASSERT_THROW(
      myComputer.addProcessor("cpu1", ProcessorType::GenericCpu, 1, 1, 1),
      DupProcessorName);

  // cannot add a container with same name
  ASSERT_THROW(myComputer.addContainer("a_container1", "cpu1", theLambda, 1),
               DupContainerName);

  // cannot add a container bound to a cpu that does not exist
  ASSERT_THROW(myComputer.addContainer("new_container", "cpuX", theLambda, 1),
               NoProcessorFound);

  // cannot add a container with the same lambda name
  ASSERT_THROW(
      myComputer.addContainer("a_containerX",
                              "cpu1",
                              Lambda("a_lambda1", FixedRequirements(1, 1)),
                              1),
      DupLambdaName);

  // clang-format off
  ASSERT_EQ(
      "computer computer1\n"
      "processor name cpu1, type GenericCpu, 1 cores, speed 100 operations/s per core, memory total/available/used 1000/1000/0 bytes, 0 running tasks\n"
      "processor name cpu2, type GenericCpu, 1 cores, speed 100 operations/s per core, memory total/available/used 1000/1000/0 bytes, 0 running tasks\n"
      "processor name cpu3, type GenericCpu, 1 cores, speed 100 operations/s per core, memory total/available/used 1000/1000/0 bytes, 0 running tasks\n"
      "container name a_container1, lambda name a_lambda1, num-workers 1, active 0, pending 0\n"
      "container name a_container2, lambda name a_lambda2, num-workers 2, active 0, pending 0\n"
      "container name a_container3, lambda name a_lambda3, num-workers 3, active 0, pending 0\n"
      "container name b_container1, lambda name b_lambda1, num-workers 1, active 0, pending 0\n"
      "container name b_container2, lambda name b_lambda2, num-workers 2, active 0, pending 0\n"
      "container name b_container3, lambda name b_lambda3, num-workers 3, active 0, pending 0\n",
      toString(myComputer));
  // clang-format on
}

TEST_F(TestComputer, test_single_container_1worker) {
  single_container(1);
}
TEST_F(TestComputer, test_single_container_5workers) {
  single_container(5);
}
TEST_F(TestComputer, test_single_container_100workers) {
  single_container(100);
}

TEST_F(TestComputer, test_multiple_containers) {
  std::list<std::pair<uint64_t, RespPtr>> myList;
  Collector                               myCollector(myList);
  Computer myComputer(theName, myCollector, Computer::UtilCallback());

  myComputer.addProcessor("cpu", ProcessorType::GenericCpu, 100, 1, 1000);
  myComputer.addContainer(
      "container_cpu", "cpu", Lambda("lambda", FixedRequirements(10, 1)), 1);

  myComputer.addProcessor("gpu", ProcessorType::GenericGpu, 1000, 1, 1000);
  myComputer.addContainer("container_gpu1",
                          "gpu",
                          Lambda("lambda_simple", FixedRequirements(10, 1)),
                          1);
  myComputer.addContainer("container_gpu2",
                          "gpu",
                          Lambda("lambda_complex", FixedRequirements(1000, 1)),
                          1);

  LambdaRequest myReqCpu("lambda", "input");
  LambdaRequest myReqGpuSimple("lambda_simple", "input");
  LambdaRequest myReqGpuComplex("lambda_complex", "input");

  std::set<uint64_t> myExpectedIds;
  myExpectedIds.insert(myComputer.addTask(myReqCpu));
  myExpectedIds.insert(myComputer.addTask(myReqCpu));
  myExpectedIds.insert(myComputer.addTask(myReqGpuSimple));
  myExpectedIds.insert(myComputer.addTask(myReqGpuSimple));
  myComputer.addTask(myReqGpuComplex);

  WAIT_FOR([&]() { return myList.size() == 4; }, 1.0);
  std::set<uint64_t> myIds;
  auto               myIt = myList.begin();
  for (; myIt != myList.end(); ++myIt) {
    myIds.insert(myIt->first);
  }
  ASSERT_EQ(myExpectedIds, myIds);

  WAIT_FOR([&]() { return myList.size() == 5; }, 1.0);
  ASSERT_EQ(4, myList.back().first);
}

} // namespace edge
} // namespace uiiit
