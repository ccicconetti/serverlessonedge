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

#include "Edge/processor.h"

#include "gtest/gtest.h"

namespace uiiit {
namespace edge {

struct TestProcessor : public ::testing::Test {
  TestProcessor()
      : theName("cpu1") {
  }
  const std::string theName;
};

TEST_F(TestProcessor, test_ctor) {
  ASSERT_NO_THROW((Processor{theName, ProcessorType::GenericCpu, 10, 1, 100}));
}

TEST_F(TestProcessor, test_allocated) {
  Processor myProc{theName, ProcessorType::GenericCpu, 10, 1, 100};

  ASSERT_EQ(100u, myProc.memTotal());
  ASSERT_EQ(0u, myProc.memUsed());
  ASSERT_EQ(100u, myProc.memAvailable());

  myProc.allocate(10);
  ASSERT_EQ(100u, myProc.memTotal());
  ASSERT_EQ(10u, myProc.memUsed());
  ASSERT_EQ(90u, myProc.memAvailable());

  myProc.allocate(90);
  ASSERT_EQ(100u, myProc.memTotal());
  ASSERT_EQ(100u, myProc.memUsed());
  ASSERT_EQ(0u, myProc.memAvailable());

  myProc.free(40);
  ASSERT_EQ(100u, myProc.memTotal());
  ASSERT_EQ(60u, myProc.memUsed());
  ASSERT_EQ(40u, myProc.memAvailable());

  ASSERT_THROW(myProc.free(61), std::runtime_error);
  ASSERT_THROW(myProc.allocate(41), std::runtime_error);
}

} // namespace edge
} // namespace uiiit
