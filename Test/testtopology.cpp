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

#include "Edge/topology.h"

#include "gtest/gtest.h"

#include <boost/filesystem.hpp>

#include <fstream>
#include <sstream>
#include <string>

namespace uiiit {
namespace edge {

struct TestTopology : public ::testing::Test {
  TestTopology()
      : theFilename(
            (boost::filesystem::path("TO_REMOVE_DIR") / "dist.txt").string()) {
  }

  void SetUp() {
    boost::filesystem::remove_all("TO_REMOVE_DIR");
    boost::filesystem::create_directories("TO_REMOVE_DIR");
  }

  void TearDown() {
    boost::filesystem::remove_all("TO_REMOVE_DIR");
  }

  const std::string theFilename;
};

TEST_F(TestTopology, test_valid) {
  {
    std::ofstream myOutputFile(theFilename);
    myOutputFile << "# example file\n"
                    "10.0.0.1 1  2  3  4\n"
                    "10.0.0.2 5  6  7  8\n"
                    "10.0.0.3 9  10 11 12\n"
                    "10.0.0.4 13 14 15 16\n";
  }

  Topology myTopology(theFilename);
  ASSERT_EQ(4u, myTopology.numNodes());

  const std::set<std::string> myAddresses({
      "10.0.0.1",
      "10.0.0.2",
      "10.0.0.3",
      "10.0.0.4",
  });

  std::stringstream myOut;
  for (const auto& mySrc : myAddresses) {
    for (const auto& myDst : myAddresses) {
      myOut << ' ' << myTopology.distance(mySrc, myDst);
    }
  }

  ASSERT_EQ(" 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16", myOut.str());
}

TEST_F(TestTopology, test_invalid) {
  // file not existing
  ASSERT_THROW(Topology("notexistingfile"), InvalidTopologyFile);

  // empty file
  { std::ofstream myOutputFile(theFilename); }
  ASSERT_THROW((Topology(theFilename)), InvalidTopologyFile);

  // all blank lines
  {
    std::ofstream myOutputFile(theFilename);
    myOutputFile << "\n\n\n";
  }
  ASSERT_THROW((Topology(theFilename)), InvalidTopologyFile);

  // all comments and blank lines
  {
    std::ofstream myOutputFile(theFilename);
    myOutputFile << "# this is a comment\n"
                    "\n"
                    "# another comment\n"
                    "\n";
  }
  ASSERT_THROW((Topology(theFilename)), InvalidTopologyFile);

  // wrong number of rows vs collum
  {
    std::ofstream myOutputFile(theFilename);
    myOutputFile << "mickey 1 2 3\n"
                    "goofy 0 0 0\n";
  }
  ASSERT_THROW((Topology(theFilename)), InvalidTopologyFile);
  {
    std::ofstream myOutputFile(theFilename);
    myOutputFile << "mickey 1\n"
                    "goofy 0\n";
  }
  ASSERT_THROW((Topology(theFilename)), InvalidTopologyFile);

  // repeated names
  {
    std::ofstream myOutputFile(theFilename);
    myOutputFile << "mickey 1 2\n"
                    "goofy 0 1\n"
                    "mickey 0 1\n";
  }
  {
    std::ofstream myOutputFile(theFilename);
    myOutputFile << "mickey 1 2 3\n"
                    "goofy 0 1 2\n"
                    "mickey 0 1 2\n";
  }
  ASSERT_THROW((Topology(theFilename)), InvalidTopologyFile);

  // file is ok, invalid access through distance()
  {
    std::ofstream myOutputFile(theFilename);
    myOutputFile << "10.0.0.1 1\n";
  }

  Topology myTopology(theFilename);
  ASSERT_EQ(1u, myTopology.numNodes());
  ASSERT_THROW(myTopology.distance("10.0.0.1", "notexistingnode"), InvalidNode);
  ASSERT_THROW(myTopology.distance("notexistingnode", "10.0.0.1"), InvalidNode);
  ASSERT_THROW(myTopology.distance("notexistingnode", "notexistingnode"),
               InvalidNode);
  ASSERT_NO_THROW(myTopology.distance("10.0.0.1", "10.0.0.1"));
}

} // namespace edge
} // namespace uiiit
