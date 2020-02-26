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

#include "Edge/composer.h"
#include "Edge/computer.h"
#include "Support/conf.h"
#include "Support/testutils.h"

#include "gtest/gtest.h"

#include <fstream>

namespace uiiit {
namespace edge {

struct TestComposer : public ::testing::Test {
  TestComposer()
      : theComputer(
            "my-computer",
            [](const uint64_t, const std::shared_ptr<const LambdaResponse>&) {},
            [](const std::map<std::string, double>&) {}) {
  }
  ~TestComposer() {
  }
  Computer theComputer;
};

TEST_F(TestComposer, test_types) {
  ASSERT_EQ(std::set<std::string>({"file", "intel-server", "raspberry"}),
            Composer::types());
}

TEST_F(TestComposer, test_intelserver) {
  ASSERT_NO_THROW(Composer()(
      support::Conf("type=intel-server,num-containers=1,num-workers=4"),
      theComputer));
}

TEST_F(TestComposer, test_raspberry) {
  ASSERT_NO_THROW(Composer()(
      support::Conf("type=raspberry,num-cpu-containers=1,num-cpu-workers=4,num-"
                    "gpu-containers=1,num-gpu-workers=2"),
      theComputer));
}

TEST_F(TestComposer, test_file) {
  support::TempDirRaii myTmpDir;
  const auto        myFilename = (myTmpDir.path() / "a.json").string();
  {
    std::ofstream myJsonFile(myFilename);
    assert(myJsonFile);
    myJsonFile << Composer::jsonExample();
  }
  ASSERT_NO_THROW(
      Composer()(support::Conf("type=file,path=" + myFilename), theComputer));
}

} // namespace edge
} // namespace uiiit
