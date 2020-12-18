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
    , theGraph() {
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

  // create the network graph
  theGraph = Graph();
}

} // namespace statesim
} // namespace uiiit
