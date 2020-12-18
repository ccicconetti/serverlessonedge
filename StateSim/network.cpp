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

#include "StateSim/network.h"

#include "Support/split.h"

#include <glog/logging.h>

#include <boost/graph/dijkstra_shortest_paths.hpp>

#include <fstream>

namespace uiiit {
namespace statesim {

struct NodeList : public std::vector<std::string> {
  static NodeList make(const std::string&             aString,
                       [[maybe_unused]] Counter<int>& aCounter) {
    const auto ret = support::split<NodeList>(aString, " ");
    if (ret.size() <= 1) {
      throw std::runtime_error("Invalid edge: " + aString);
    }
    return ret;
  }
};

template <class T>
std::vector<T> loadFile(const std::string& aPath, Counter<int>& aCounter) {
  std::vector<T> ret;
  std::ifstream  myFile(aPath);
  if (not myFile) {
    throw std::runtime_error("Cannot open file for reading: " + aPath);
  }
  std::string myLine;
  while (myFile) {
    std::getline(myFile, myLine);
    if (myLine.empty() or myLine[0] == '#') {
      continue;
    }
    ret.emplace_back(T::make(myLine, aCounter));
  }
  return ret;
}

std::vector<Node> loadNodes(const std::string& aPath, Counter<int>& aCounter) {
  return loadFile<Node>(aPath, aCounter);
}

std::vector<Link> loadLinks(const std::string& aPath, Counter<int>& aCounter) {
  return loadFile<Link>(aPath, aCounter);
}

std::vector<NodeList> loadNodeLists(const std::string& aPath) {
  Counter<int> myCounter;
  return loadFile<NodeList>(aPath, myCounter);
}

////////////////////////////////////////////////////////////////////////////////
// class Network

Network::Network(const std::string& aNodesPath,
                 const std::string& aLinksPath,
                 const std::string& aEdgesPath)
    : theNodes()
    , theLinks()
    , theElements()
    , theGraph()
    , theCache() {
  // read from files
  Counter<int> myCounter;
  auto         myNodes     = loadNodes(aNodesPath, myCounter);
  auto         myLinks     = loadLinks(aLinksPath, myCounter);
  const auto   myNodeLists = loadNodeLists(aEdgesPath);

  // copy into member maps
  for (const auto& myNode : myNodes) {
    theNodes.emplace(myNode.name(), myNode);
  }
  for (const auto& myLink : myLinks) {
    theLinks.emplace(myLink.name(), myLink);
  }

  // add to theNodes the non-processing nodes
  for (const auto& myNodeList : myNodeLists) {
    for (const auto& myName : myNodeList) {
      if (theNodes.find(myName) == theNodes.end() and
          theLinks.find(myName) == theLinks.end()) {
        [[maybe_unused]] const auto ret =
            theNodes.emplace(myName, Node(myName, myCounter()));
        assert(ret.second);
      }
    }
  }

  // create the list of edges (with weights)
  std::vector<Edge>  myEdges;
  std::vector<float> myWeights;
  for (const auto& myNodeList : myNodeLists) {
    assert(myNodeList.size() >= 2);
    auto       it               = myNodeList.begin();
    const auto mySourceId       = id(*it);
    const auto mySourceCapacity = capacity(*it);
    const auto mySourceLatency =
        mySourceCapacity > 0 ? (2.0f / mySourceCapacity) : 0;
    ++it;
    for (; it != myNodeList.end(); ++it) {
      const auto myTargetId       = id(*it);
      const auto myTargetCapacity = capacity(*it);

      // compute the weight as the sum of the half reciprocals of the source
      // and target, but only if they are links
      const auto myWeight =
          mySourceLatency +
          (myTargetCapacity > 0 ? (2.0f / myTargetCapacity) : 0);

      myEdges.emplace_back(Edge(mySourceId, myTargetId));
      myWeights.emplace_back(myWeight);

      VLOG(2) << mySourceId << ' ' << myTargetId << ' ' << myWeight;
    }
  }

  // fill the nodes/links vector indexed by the identifiers
  theElements.resize(theNodes.size() + theLinks.size());
  for (auto& myNode : theNodes) {
    assert(myNode.second.id() < theElements.size());
    theElements[myNode.second.id()] = &myNode.second;
  }
  for (auto& myLink : theLinks) {
    assert(myLink.second.id() < theElements.size());
    theElements[myLink.second.id()] = &myLink.second;
  }
  for (const auto& elem : theElements) {
    assert(elem != nullptr);
  }

  // create the network graph
  theGraph = Graph(myEdges.data(),
                   myEdges.data() + myEdges.size(),
                   myWeights.data(),
                   theNodes.size() + theLinks.size());
  VLOG(1) << "Created a network with " << theNodes.size() << " nodes and "
          << theLinks.size() << " links";
  assert(theElements.size() == boost::num_vertices(theGraph));
  theCache.resize(theElements.size(), {false, PredVec()});
}

std::pair<float, std::string> Network::nextHop(const std::string& aSrc,
                                               const std::string& aDst) {
  const auto mySrcId = id(aSrc);
  const auto myDstId = id(aDst);
  assert(mySrcId >= 0);
  assert(mySrcId < (int)theCache.size());

  auto& myCacheEntry = theCache[mySrcId];
  if (myCacheEntry.first == false) {
    assert(myCacheEntry.second.size() == 0);

    // create an entry in the cache for this source

    std::vector<VertexDescriptor> myPred(theCache.size());
    std::vector<float>            myDist(theCache.size());

    boost::dijkstra_shortest_paths(
        theGraph,
        mySrcId,
        boost::predecessor_map(
            boost::make_iterator_property_map(
                myPred.begin(), get(boost::vertex_index, theGraph)))
            .distance_map(myDist.data()));

    myCacheEntry.first = true;
    myCacheEntry.second.resize(theCache.size());
    for (size_t i = 0; i < theCache.size(); i++) {
      myCacheEntry.second[i] = {myDist[i], myPred[i]};
    }
  }
  assert(myDstId >= 0);
  assert(myDstId < (int)myCacheEntry.second.size());
  const auto& myRet = myCacheEntry.second[myDstId];
  assert(myRet.second < theElements.size());
  assert(theElements[myRet.second] != nullptr);
  return {myRet.first, theElements[myRet.second]->name()};
}

int Network::id(const std::string& aName) const {
  const auto it = theNodes.find(aName);
  if (it != theNodes.end()) {
    return it->second.id();
  }
  const auto jt = theLinks.find(aName);
  if (jt != theLinks.end()) {
    return jt->second.id();
  }
  throw std::runtime_error("Unknown node with name: " + aName);
}

float Network::capacity(const std::string& aName) const {
  const auto it = theNodes.find(aName);
  if (it != theNodes.end()) {
    return 0;
  }
  const auto jt = theLinks.find(aName);
  if (jt != theLinks.end()) {
    return jt->second.capacity();
  }
  throw std::runtime_error("Unknown node with name: " + aName);
}

} // namespace statesim
} // namespace uiiit
