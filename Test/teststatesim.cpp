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

#include "StateSim/counter.h"
#include "StateSim/job.h"
#include "StateSim/network.h"

#include "gtest/gtest.h"

#include <boost/filesystem.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>

#include <glog/logging.h>

namespace uiiit {
namespace statesim {

#include "Test/Data/datastatesim.h"
#include "Test/Data/taskstatesim.h"

struct TestStateSim : public ::testing::Test {
  TestStateSim()
      : theTestDir("TO_REMOVE_DIR") {
  }

  void SetUp() {
    boost::filesystem::remove_all(theTestDir);
    boost::filesystem::create_directories(theTestDir);
  }

  void TearDown() {
    // boost::filesystem::remove_all(theTestDir);
  }

  bool prepareNetworkFiles() const {
    std::ofstream myEdges((theTestDir / "edges").string());
    myEdges << theEdges;

    std::ofstream myLinks((theTestDir / "links").string());
    myLinks << theLinks;

    std::ofstream myNodes((theTestDir / "nodes").string());
    myNodes << theNodes;

    std::ofstream myGraph((theTestDir / "graph").string());
    myGraph << theGraph;

    return static_cast<bool>(myEdges) and static_cast<bool>(myLinks) and
           static_cast<bool>(myNodes) and static_cast<bool>(myGraph);
  }

  bool prepareTaskFiles() const {
    std::ofstream myTasks((theTestDir / "tasks").string());
    myTasks << theTasks;
    return static_cast<bool>(myTasks);
  }

  const boost::filesystem::path theTestDir;
};

TEST_F(TestStateSim, test_boost_graph) {
  typedef boost::adjacency_list<boost::listS,
                                boost::vecS,
                                boost::directedS,
                                boost::no_property,
                                boost::property<boost::edge_weight_t, float>,
                                boost::no_property,
                                boost::listS>
                                                        Graph;
  typedef boost::graph_traits<Graph>::vertex_descriptor VertexDescriptor;
  typedef std::pair<int, int>                           Edge;

  std::vector<Edge>  myEdges({
      {0, 1},
      {1, 0},
      {1, 2},
      {2, 1},
      {2, 3},
      {3, 2},
      {3, 4},
      {4, 3},
      {0, 4},
      {4, 0},
  });
  std::vector<float> myWeights({1, 1, 1, 1, 1, 1, 0.5, 0.5, 0.5, 0.5});

  Graph myGraph(
      myEdges.data(), myEdges.data() + myEdges.size(), myWeights.data(), 5);

  // vector for storing distance property
  std::vector<float> myDistances(boost::num_vertices(myGraph));

  // vector for storing the predecessor map
  std::vector<VertexDescriptor> myPredecessors(boost::num_vertices(myGraph));

  // get the first vertex
  const auto mySource = *(vertices(myGraph).first);
  // invoke variant 2 of Dijkstra's algorithm
  boost::dijkstra_shortest_paths(
      myGraph,
      mySource,
      boost::predecessor_map(
          boost::make_iterator_property_map(myPredecessors.begin(),
                                            get(boost::vertex_index, myGraph)))
          .distance_map(myDistances.data()));

  std::vector<float>            myExpectedDistances({0, 1, 2, 1, 0.5});
  std::vector<VertexDescriptor> myExpectedPredecessors({0, 0, 1, 4, 0});
  for (auto vi = boost::vertices(myGraph).first; vi != vertices(myGraph).second;
       ++vi) {
    EXPECT_FLOAT_EQ(myExpectedDistances[*vi], myDistances[*vi])
        << "node " << *vi;
    EXPECT_EQ(myExpectedPredecessors[*vi], myPredecessors[*vi])
        << "node " << *vi;
    VLOG(1) << *vi << ' ' << myDistances[*vi] << ' ' << myPredecessors[*vi];
  }
}

TEST_F(TestStateSim, test_network_files) {
  Counter<int> myCounter;
  ASSERT_THROW(loadNodes((theTestDir / "nodes").string(), myCounter),
               std::runtime_error);
  ASSERT_THROW(loadLinks((theTestDir / "links").string(), myCounter),
               std::runtime_error);

  ASSERT_TRUE(prepareNetworkFiles());

  const auto myNodes = loadNodes((theTestDir / "nodes").string(), myCounter);
  ASSERT_EQ(123, myNodes.size());
  const auto myLinks = loadLinks((theTestDir / "links").string(), myCounter);
  ASSERT_EQ(140, myLinks.size());
  ASSERT_EQ(myNodes.size() + myLinks.size() - 1, myLinks.back().id());
}

TEST_F(TestStateSim, test_network) {
  ASSERT_TRUE(prepareNetworkFiles());
  Network myNetwork((theTestDir / "nodes").string(),
                    (theTestDir / "links").string(),
                    (theTestDir / "edges").string());
  ASSERT_EQ(140, myNetwork.links().size());
  ASSERT_EQ(159, myNetwork.nodes().size());

  const auto myDesc = myNetwork.nextHop("rpi3_0", "rpi3_53");
  ASSERT_FLOAT_EQ(0.216f, myDesc.first);
  ASSERT_EQ("link_rpi3_53", myDesc.second);

  for (const auto& mySrc : myNetwork.nodes()) {
    for (const auto& myDst : myNetwork.nodes()) {
      const auto myDesc = myNetwork.nextHop(mySrc.first, myDst.first);
      ASSERT_GE(myDesc.first, 0.0f);
      VLOG(1) << mySrc.first << " -> " << myDst.first << ": " << myDesc.first
              << " (" << myDesc.second << ")";
    }
  }
}

TEST_F(TestStateSim, test_tasks) {
  ASSERT_TRUE(prepareTaskFiles());
  const auto myJobs = loadJobs((theTestDir / "tasks").string(), 1000, 100, 42);
  ASSERT_EQ(22, myJobs.size());

  const auto& myJob = myJobs[10];
  ASSERT_EQ(10, myJob.id());
  ASSERT_FLOAT_EQ(3.0875, myJob.start());
  ASSERT_EQ(30, myJob.retSize());
  ASSERT_EQ(7, myJob.tasks().size());
  ASSERT_EQ(40000, myJob.tasks()[3].ops());
  ASSERT_EQ(59, myJob.tasks()[3].size());
  ASSERT_EQ(std::vector<size_t>({0, 1, 2}), myJob.tasks()[3].deps());
  ASSERT_EQ(std::vector<size_t>({39, 59, 49}), myJob.stateSizes());
}

} // namespace statesim
} // namespace uiiit
