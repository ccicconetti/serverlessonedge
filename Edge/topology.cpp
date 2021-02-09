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

#include "topology.h"

#include "Support/random.h"

#include <cassert>
#include <fstream>
#include <sstream>

#include <iostream>

namespace uiiit {
namespace edge {

Topology::Topology(const std::string& aInputFile) {
  std::ifstream myInputFile(aInputFile);
  if (not myInputFile) {
    throw InvalidTopologyFile(aInputFile);
  }

  int         myNumNodes = -1;
  size_t      myCurNode  = 0;
  std::string myLine;
  while (not myInputFile.eof()) {
    std::getline(myInputFile, myLine);

    if (not myInputFile) {
      break;
    }

    if (myLine.empty() or myLine[0] == '#') {
      continue;
    }

    if (myNumNodes > 0 and (int) myCurNode >= myNumNodes) {
      throw InvalidTopologyFile(aInputFile);
    }

    std::stringstream myStream;
    myStream << myLine;

    std::string myName;
    myStream >> myName;

    if (myName.empty()) {
      throw InvalidTopologyFile(aInputFile);
    }

    if (not theNames.emplace(myName, myCurNode).second) {
      throw InvalidTopologyFile(aInputFile);
    }

    std::vector<double> myRow;
    double              myDistance;
    while (not myStream.eof()) {
      myStream >> myDistance;
      if (not myStream) {
        break;
      }
      myRow.push_back(myDistance);
    }

    if (myNumNodes < 0) {
      if (myRow.empty()) {
        throw InvalidTopologyFile(aInputFile);
      } else {
        myNumNodes = myRow.size();
        theDistances.resize(myNumNodes * myNumNodes);
      }
    } else if (myNumNodes != (int)myRow.size()) {
      throw InvalidTopologyFile(aInputFile);
    }

    // copy row elements into the matrix
    for (size_t i = 0; i < myRow.size(); i++) {
      assert(myCurNode * myRow.size() + i < theDistances.size());
      theDistances[myCurNode * myRow.size() + i] = myRow[i];
    }

    myCurNode++;
  }
  assert(myCurNode == theNames.size());

  if (theDistances.empty()) {
    throw InvalidTopologyFile(aInputFile);
  }

  if (theDistances.size() != (theNames.size() * theNames.size())) {
    throw InvalidTopologyFile(aInputFile);
  }
}

Topology::~Topology() {
  // nihil
}

size_t Topology::numNodes() const {
  return theNames.size();
}

double Topology::distance(const std::string& aSrc,
                          const std::string& aDst) const {
  const auto mySrcIt = theNames.find(aSrc);
  if (mySrcIt == theNames.end()) {
    throw InvalidNode(aSrc);
  }
  const auto myDstIt = theNames.find(aDst);
  if (myDstIt == theNames.end()) {
    throw InvalidNode(aDst);
  }

  const auto myDistNdx = mySrcIt->second * theNames.size() + myDstIt->second;
  assert(myDistNdx < theDistances.size());
  return theDistances[myDistNdx];
}

void Topology::randomize() {
  const auto N = theNames.size(); // alias
  for (size_t i = 0; i < N; i++) {
    for (size_t j = 0; j < N; j++) {
      if (i != j) {
        theDistances[i * N + j] = support::random();
      }
    }
  }
}

void Topology::print(std::ostream& aStream) const {
  const auto N = theNames.size(); // alias

  std::vector<std::string> myNames(theNames.size());
  for (const auto& elem : theNames) {
    assert(elem.second < N);
    myNames[elem.second] = elem.first;
  }

  for (size_t i = 0; i < N; i++) {
    aStream << myNames[i];
    for (size_t j = 0; j < N; j++) {
      assert(i * N + j < theDistances.size());
      aStream << ' ' << theDistances[i * N + j];
    }
    aStream << '\n';
  }
}

} // end namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                aStream,
                         const uiiit::edge::Topology& aTopology) {
  aTopology.print(aStream);
  return aStream;
}
