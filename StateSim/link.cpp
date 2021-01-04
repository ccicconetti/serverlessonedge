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

#include "StateSim/link.h"

#include "Support/split.h"

#include <cassert>
#include <map>
#include <sstream>
#include <vector>

namespace uiiit {
namespace statesim {

Link::Link(const Type         aType,
           const std::string& aName,
           const size_t       aId,
           const float        aCapacity)
    : Element(aName, aId)
    , theType(aType)
    , theCapacity(aCapacity) {
  if (aCapacity <= 0) {
    throw std::runtime_error("Invalid capacity (" + std::to_string(aCapacity) +
                             ") for Link name: " + aName);
  }
}

Link Link::make(const std::string& aString, Counter<int>& aCounter) {
  static const std::map<std::string, Type> myTypes({
      {"node", Type::Node},
      {"shared", Type::Shared},
      {"downlink", Type::Downlink},
      {"uplink", Type::Uplink},
  });
  const auto myTokens = support::split<std::vector<std::string>>(aString, ";");
  if (myTokens.size() != 4) {
    throw std::runtime_error("Invalid Link: " + aString);
  }
  const auto myType = myTypes.find(myTokens[3]);
  if (myType == myTypes.end()) {
    throw std::runtime_error("Invalid type in Link: " + aString);
  }
  return Link(myType->second, myTokens[2], aCounter(), std::stof(myTokens[1]));
}

std::string Link::toString() const {
  static const std::map<Type, char> myTypes({
      {Type::Node, 'n'},
      {Type::Shared, 's'},
      {Type::Downlink, 'd'},
      {Type::Uplink, 'u'},
  });

  const auto myType = myTypes.find(theType);
  assert(myType != myTypes.end());

  std::stringstream ret;
  ret << myType->second << ' ' << theId << ' ' << theName << ", capacity "
      << theCapacity << " Mb/s";
  return ret.str();
}

} // namespace statesim
} // namespace uiiit
