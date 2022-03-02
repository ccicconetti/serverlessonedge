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

#include "hungarian-algorithm-cpp/Hungarian.h"

#include "gtest/gtest.h"

#include <vector>

namespace hungarian {

struct TestHungarian : public ::testing::Test {};

TEST_F(TestHungarian, test_original_example) {
  // numbers from:
  // https://github.com/mcximing/hungarian-algorithm-cpp/blob/master/testMain.cpp

  std::vector<std::vector<double>> myCostMatrix = {{10, 19, 8, 15, 0},
                                                   {10, 18, 7, 17, 0},
                                                   {13, 16, 9, 14, 0},
                                                   {12, 19, 8, 18, 0}};
  std::vector<int>                 myAssignment;

  ASSERT_FLOAT_EQ(31, HungarianAlgorithm::Solve(myCostMatrix, myAssignment));
  ASSERT_EQ(std::vector<int>({0, 2, 3, 4}), myAssignment);
}

TEST_F(TestHungarian, test_ortools_example) {
  // numbers from:
  // https://developers.google.com/optimization/assignment/assignment_example

  std::vector<std::vector<double>> myCostMatrix = {
      {90, 80, 75, 70},
      {35, 85, 55, 65},
      {125, 95, 90, 95},
      {45, 110, 95, 115},
      {50, 100, 90, 100},
  };
  std::vector<int> myAssignment;

  ASSERT_FLOAT_EQ(265, HungarianAlgorithm::Solve(myCostMatrix, myAssignment));
  ASSERT_EQ(std::vector<int>({3, 2, 1, 0, -1}), myAssignment);
}

} // namespace hungarian
