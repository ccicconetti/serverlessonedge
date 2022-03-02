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

#include "gtest/gtest.h"

#include <stdexcept>
#include <vector>

namespace uiiit {
namespace lambdamusim {

struct TestMcfp : public ::testing::Test {};

TEST_F(TestMcfp, test_invalid) {
  ASSERT_NO_THROW(Mcfp::solve(
      Mcfp::Costs({{1}}), Mcfp::Requests({1}), Mcfp::Capacities({1})));

  ASSERT_THROW(
      Mcfp::solve(Mcfp::Costs({}), Mcfp::Requests({}), Mcfp::Capacities({})),
      std::runtime_error);

  ASSERT_NO_THROW(Mcfp::solve(Mcfp::Costs({
                                  {1, 1},
                                  {1, 1},
                                  {1, 1},
                              }),
                              Mcfp::Requests({1, 2, 3}),
                              Mcfp::Capacities({1, 2})));

  ASSERT_THROW(Mcfp::solve(Mcfp::Costs({
                               {1, 1},
                               {1, 1},
                               {1, 1},
                           }),
                           Mcfp::Requests({1, 2}),
                           Mcfp::Capacities({1, 2})),
               std::runtime_error);

  ASSERT_THROW(Mcfp::solve(Mcfp::Costs({
                               {1, 1},
                               {1, 1},
                               {1, 1},
                           }),
                           Mcfp::Requests({1, 2, 3}),
                           Mcfp::Capacities({1, 2, 3})),
               std::runtime_error);

  ASSERT_THROW(Mcfp::solve(Mcfp::Costs({
                               {1, 1},
                               {1, 1},
                               {1, 1},
                           }),
                           Mcfp::Requests({1, 2, 3, 4}),
                           Mcfp::Capacities({1, 2, 3})),
               std::runtime_error);
}

TEST_F(TestMcfp, test_simple_problem_instances) {
  ASSERT_EQ(2,
            Mcfp::solve(Mcfp::Costs({
                            {1, 2},
                            {2, 1},
                            {3, 3},
                        }),
                        Mcfp::Requests({1, 1, 1}),
                        Mcfp::Capacities({1, 1})));

  ASSERT_EQ(5,
            Mcfp::solve(Mcfp::Costs({
                            {1, 2},
                            {2, 1},
                            {3, 3},
                        }),
                        Mcfp::Requests({1, 1, 1}),
                        Mcfp::Capacities({10, 10})));

  ASSERT_EQ(3,
            Mcfp::solve(Mcfp::Costs({
                            {0, 0},
                            {0, 0},
                            {3, 3},
                        }),
                        Mcfp::Requests({10, 10, 1}),
                        Mcfp::Capacities({11, 11})));
}

} // namespace lambdamusim
} // namespace uiiit
