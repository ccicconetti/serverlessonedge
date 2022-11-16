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

#include "LambdaMuSim/mcfp.h"

#include "Test/fakerandom.h"

#include "gtest/gtest.h"

#include <stdexcept>
#include <vector>

namespace uiiit {
namespace lambdamusim {

struct TestMcfp : public ::testing::Test {
  TestMcfp()
      : theCosts({
            {10, 5, 20, 17},
            {4, 10, 2, 30},
            {20, 4, 50, 12},
            {100, 2, 40, 25},
            {10, 10, 10, 10},
        })
      , theRequests({1, 2, 4, 2, 2})
      , theCapacities({2, 2, 3, 3}) {
    // noop
  }

  Mcfp::Costs      theCosts;
  Mcfp::Requests   theRequests;
  Mcfp::Capacities theCapacities;
};

TEST_F(TestMcfp, test_invalid) {
  Mcfp::Weights myWeights;
  ASSERT_NO_THROW(Mcfp::solve(Mcfp::Costs({{1}}),
                              Mcfp::Requests({1}),
                              Mcfp::Capacities({1}),
                              myWeights));

  ASSERT_THROW(
      Mcfp::solve(
          Mcfp::Costs({}), Mcfp::Requests({}), Mcfp::Capacities({}), myWeights),
      std::runtime_error);

  ASSERT_NO_THROW(Mcfp::solve(Mcfp::Costs({
                                  {1, 1},
                                  {1, 1},
                                  {1, 1},
                              }),
                              Mcfp::Requests({1, 2, 3}),
                              Mcfp::Capacities({1, 2}),
                              myWeights));

  ASSERT_THROW(Mcfp::solve(Mcfp::Costs({
                               {1, 1},
                               {1, 1},
                               {1, 1},
                           }),
                           Mcfp::Requests({1, 2}),
                           Mcfp::Capacities({1, 2}),
                           myWeights),
               std::runtime_error);

  ASSERT_THROW(Mcfp::solve(Mcfp::Costs({
                               {1, 1},
                               {1, 1},
                               {1, 1},
                           }),
                           Mcfp::Requests({1, 2, 3}),
                           Mcfp::Capacities({1, 2, 3}),
                           myWeights),
               std::runtime_error);

  ASSERT_THROW(Mcfp::solve(Mcfp::Costs({
                               {1, 1},
                               {1, 1},
                               {1, 1},
                           }),
                           Mcfp::Requests({1, 2, 3, 4}),
                           Mcfp::Capacities({1, 2, 3}),
                           myWeights),
               std::runtime_error);
}

TEST_F(TestMcfp, test_simple_problem_instances) {
  Mcfp::Weights myWeights;
  ASSERT_FLOAT_EQ(2,
                  Mcfp::solve(Mcfp::Costs({
                                  {1, 2},
                                  {2, 1},
                                  {3, 3},
                              }),
                              Mcfp::Requests({1, 1, 1}),
                              Mcfp::Capacities({1, 1}),
                              myWeights));

  ASSERT_EQ(3, myWeights.size());
  ASSERT_EQ(std::vector<double>({1, 0}), myWeights[0]);
  ASSERT_EQ(std::vector<double>({0, 1}), myWeights[1]);
  ASSERT_EQ(std::vector<double>({0, 0}), myWeights[2]);

  ASSERT_FLOAT_EQ(5,
                  Mcfp::solve(Mcfp::Costs({
                                  {1, 2},
                                  {2, 1},
                                  {3, 3},
                              }),
                              Mcfp::Requests({1, 1, 1}),
                              Mcfp::Capacities({10, 10}),
                              myWeights));

  ASSERT_FLOAT_EQ(3,
                  Mcfp::solve(Mcfp::Costs({
                                  {0, 0},
                                  {0, 0},
                                  {3, 3},
                              }),
                              Mcfp::Requests({10, 10, 1}),
                              Mcfp::Capacities({11, 11}),
                              myWeights));

  ASSERT_FLOAT_EQ(1.7,
                  Mcfp::solve(Mcfp::Costs({
                                  {1.5, .1},
                                  {2, .1},
                                  {3, .1},
                              }),
                              Mcfp::Requests({1, 1, 1}),
                              Mcfp::Capacities({2, 2}),
                              myWeights));
}

TEST_F(TestMcfp, test_solver) {
  Mcfp::Weights myWeights;
  EXPECT_FLOAT_EQ(74,
                  Mcfp::solve(theCosts, theRequests, theCapacities, myWeights));
  EXPECT_EQ(Mcfp::Weights({
                {1, 0, 0, 0},
                {0, 0, 2, 0},
                {0, 0, 0, 3},
                {0, 2, 0, 0},
                {1, 0, 1, 0},
            }),
            myWeights);
}

TEST_F(TestMcfp, test_solver_random) {
  ::FakeRandom  myFakeRandom;
  Mcfp::Weights myWeights;
  EXPECT_FLOAT_EQ(
      156,
      Mcfp::solveRandom(
          theCosts, theRequests, theCapacities, myWeights, myFakeRandom()));
  EXPECT_EQ(Mcfp::Weights({
                {0, 0, 1, 0},
                {1, 0, 1, 0},
                {0, 2, 1, 1},
                {0, 0, 0, 2},
                {1, 0, 0, 0},
            }),
            myWeights);
  EXPECT_FLOAT_EQ(
      238,
      Mcfp::solveRandom(
          theCosts, theRequests, theCapacities, myWeights, myFakeRandom()));
  EXPECT_EQ(Mcfp::Weights({
                {1, 0, 0, 0},
                {1, 1, 0, 0},
                {0, 1, 3, 0},
                {0, 0, 0, 2},
                {0, 0, 0, 1},
            }),
            myWeights);
  EXPECT_FLOAT_EQ(
      249,
      Mcfp::solveRandom(
          theCosts, theRequests, theCapacities, myWeights, myFakeRandom()));
  EXPECT_EQ(Mcfp::Weights({
                {0, 0, 0, 0},
                {0, 1, 0, 1},
                {1, 1, 2, 0},
                {0, 0, 1, 1},
                {1, 0, 0, 1},
            }),
            myWeights);
}

TEST_F(TestMcfp, test_solver_greedy) {
  Mcfp::Weights myWeights;
  EXPECT_FLOAT_EQ(
      199, Mcfp::solveGreedy(theCosts, theRequests, theCapacities, myWeights));
  EXPECT_EQ(Mcfp::Weights({
                {0, 1, 0, 0},
                {0, 0, 2, 0},
                {0, 1, 0, 3},
                {1, 0, 1, 0},
                {1, 0, 0, 0},
            }),
            myWeights);

  Mcfp::Weights myOtherWeights;
  EXPECT_FLOAT_EQ(
      199,
      Mcfp::solveGreedy(theCosts, theRequests, theCapacities, myOtherWeights));
  ASSERT_EQ(myWeights, myOtherWeights);
}

} // namespace lambdamusim
} // namespace uiiit
