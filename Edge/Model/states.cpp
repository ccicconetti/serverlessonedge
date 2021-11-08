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

#include "Edge/Model/states.h"

#include "Support/tostring.h"

#include <nlohmann/json.hpp>

#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

namespace uiiit {
namespace edge {
namespace model {

States::States(const Dependencies& aDependencies)
    : theDependencies(aDependencies) {
  // noop
}

States::~States() {
  // noop
}

bool States::operator==(const States& aOther) const {
  return theDependencies == aOther.theDependencies;
}

const States::Dependencies& States::dependencies() const {
  return theDependencies;
}

std::set<std::string> States::allStates(const bool aIncludeFreeStates) const {
  std::set<std::string> ret;
  for (const auto& elem : theDependencies) {
    if (aIncludeFreeStates or not elem.second.empty()) {
      ret.insert(elem.first);
    }
  }
  return ret;
}

std::set<std::string> States::states(const std::string& aFunction) const {
  std::set<std::string> ret;
  for (const auto& elem : theDependencies) {
    if (std::find(elem.second.begin(), elem.second.end(), aFunction) !=
        elem.second.end()) {
      ret.insert(elem.first);
    }
  }
  return ret;
}

std::string States::toString() const {
  std::stringstream ret;
  ret << "{";
  for (const auto& elem : theDependencies) {
    ret << "(" << elem.first << ": " << ::toString(elem.second, ",") << ") ";
  }
  ret << "}";
  return ret.str();
}

States States::fromJson(const std::string& aJson) {
  const auto   myJson = json::parse(aJson);
  Dependencies myDependencies;
  for (const auto& elem : myJson["dependencies"].items()) {
    if (elem.value().is_null()) {
      myDependencies.emplace(elem.key(), Dependencies::mapped_type());
    } else {
      myDependencies.emplace(elem.key(), elem.value());
    }
  }
  return States(myDependencies);
}

std::string States::toJson() const {
  json  ret;
  auto& myDependencies = ret["dependencies"];
  for (const auto& elem : theDependencies) {
    auto& myList = myDependencies[elem.first];
    for (const auto& myFunction : elem.second) {
      myList.push_back(myFunction);
    }
  }
  return ret.dump(2);
}

} // namespace model
} // namespace edge
} // namespace uiiit
