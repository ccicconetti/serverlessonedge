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

#include "StateSim/node.h"

#include "Support/split.h"

#include <map>
#include <sstream>
#include <vector>

namespace uiiit {
namespace statesim {

Node::Node(const std::string& aName, const size_t aId)
    : Element(aName, aId, Device::Node)
    , theType(Type::Networking)
    , theSpeed(0)
    , theMemory(0)
    , theAffinity(Affinity::NotAvailable)
    , theClient(false) {
  // noop
}

Node::Node(const std::string& aName,
           const size_t       aId,
           const float        aSpeed,
           const size_t       aMemory,
           const Affinity     aAffinity,
           const bool         aClient)
    : Element(aName, aId, Device::Node)
    , theType(Type::Processing)
    , theSpeed(aSpeed)
    , theMemory(aMemory)
    , theAffinity(aAffinity)
    , theClient(aClient) {
  if (aSpeed <= 0) {
    throw std::runtime_error("Invalid speed (" + std::to_string(aSpeed) +
                             ") for Node name: " + aName);
  }
}

Node Node::make(const std::string& aString, Counter<int>& aCounter) {
  static const std::map<std::string, std::pair<Affinity, bool>>
             myAffinitiesClients({
          {"server", {Affinity::Cpu, false}},
          {"rpi3b+", {Affinity::Cpu, true}},
          {"nuci5", {Affinity::Cpu, false}},
          {"nvidia_jetson_tx2", {Affinity::Gpu, true}},
      });
  const auto myTokens = support::split<std::vector<std::string>>(aString, ";");
  if (myTokens.size() != 7) {
    throw std::runtime_error("Invalid node: " + aString);
  }
  const auto myElem = myAffinitiesClients.find(myTokens[6]);
  if (myElem == myAffinitiesClients.end()) {
    throw std::runtime_error("Invalid node type: " + aString);
  }
  return Node(myTokens[1],
              aCounter(),
              std::stof(myTokens[3]) * 1e6,
              std::stoull(myTokens[2]),
              myElem->second.first,
              myElem->second.second);
}

std::string Node::toString() const {
  std::stringstream ret;
  ret << (theType == Type::Networking ? 'N' : 'P') << ' ' << theId << ' '
      << theName;
  if (theType == Type::Processing) {
    ret << " speed " << theSpeed / 1e9 << " GFLOPS, memory " << theMemory / 1e9
        << " GB, affinity " << ::uiiit::statesim::toString(theAffinity) << ", "
        << (theClient ? "client" : "server");
  }
  return ret.str();
}

} // namespace statesim
} // namespace uiiit
