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

#include "StateSim/node.h"

#include "Support/split.h"

#include <map>
#include <sstream>
#include <vector>

namespace uiiit {
namespace statesim {

Node::Node(const std::string& aName)
    : theType(Type::Networking)
    , theName(aName)
    , theSpeed(0)
    , theMemory(0)
    , theAffinity(Affinity::NotAvailable) {
}

Node::Node(const std::string& aName,
           const float        aSpeed,
           const size_t       aMemory,
           const Affinity     aAffinity)
    : theType(Type::Processing)
    , theName(aName)
    , theSpeed(aSpeed)
    , theMemory(aMemory)
    , theAffinity(aAffinity) {
}

Node Node::make(const std::string& aString) {
  static const std::map<std::string, Affinity> myAffinities({
      {"server", Affinity::Cpu},
      {"rpi3b+", Affinity::Cpu},
      {"nuci5", Affinity::Cpu},
      {"nvidia_jetson_tx2", Affinity::Gpu},
  });
  const auto myTokens = support::split<std::vector<std::string>>(aString, ";");
  if (myTokens.size() != 7) {
    throw std::runtime_error("Invalid node: " + aString);
  }
  const auto myAffinity = myAffinities.find(myTokens[6]);
  if (myAffinity == myAffinities.end()) {
    throw std::runtime_error("Invalid affinity in node: " + aString);
  }
  return Node(myTokens[1],
              std::stof(myTokens[2]),
              std::stoull(myTokens[3]) * 1000000,
              myAffinity->second);
}

std::string Node::toString() const {
  std::stringstream ret;
  ret << (theType == Type::Networking ? 'N' : 'P') << ' ' << theName;
  if (theType == Type::Processing) {
    ret << "speed " << theSpeed / 1e9 << " GFLOPS, memory " << theMemory / 1e9
        << " GB, affinity"
        << (theAffinity == Affinity::NotAvailable ?
                "N/A" :
                theAffinity == Affinity::Cpu ? "CPU" : "GPU");
  }
  return ret.str();
}

} // namespace statesim
} // namespace uiiit
