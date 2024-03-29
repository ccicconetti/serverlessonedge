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

#include "Edge/Model/chain.h"

#include "Support/tostring.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

namespace uiiit {
namespace edge {
namespace model {

Chain::Chain(const Functions& aFunctions, const Dependencies& aDependencies)
    : theFunctions(aFunctions)
    , theStates(aDependencies) {
  // consistency check
  std::set<std::string> myFunctions;
  for (const auto& myFunction : aFunctions) {
    myFunctions.insert(myFunction);
  }
  for (const auto& elem : aDependencies) {
    for (const auto& myFunction : elem.second) {
      if (myFunctions.count(myFunction) == 0) {
        throw std::runtime_error(
            "could not find the following function that state '" + elem.first +
            "' depends on: " + myFunction);
      }
    }
  }
}

Chain::~Chain() {
  // noop
}

bool Chain::operator==(const Chain& aOther) const {
  return theFunctions == aOther.theFunctions and theStates == aOther.theStates;
}

std::string Chain::name() const {
  return ::toString(theFunctions, "-");
}

std::set<std::string> Chain::uniqueFunctions() const {
  std::set<std::string> ret;
  for (const auto& myFunction : theFunctions) {
    ret.insert(myFunction);
  }
  return ret;
}

const Chain::Functions& Chain::functions() const {
  return theFunctions;
}

const States& Chain::states() const {
  return theStates;
}

std::string Chain::entryFunctionName() const {
  if (theFunctions.empty()) {
    throw std::runtime_error("Empty chain");
  }
  return theFunctions.front();
}

std::string Chain::toString() const {
  std::stringstream ret;
  ret << "[ " << ::toString(theFunctions, " -> ") << " ], "
      << theStates.toString();
  return ret.str();
}

Chain Chain::singleFunctionChain(const std::string& aFunction) {
  return Chain({aFunction}, theStates.dependencies(aFunction));
}

Chain Chain::fromJson(const std::string& aJson) {
  const auto myJson = json::parse(aJson);
  if (myJson.find("functions") == myJson.end()) {
    throw std::runtime_error("Invalid JSON content for a chain");
  }
  Functions  myFunctions(myJson["functions"]);
  const auto myStates = States::fromJson(aJson);
  return Chain(myFunctions, myStates.dependencies());
}

std::string Chain::toJson() const {
  json  ret;
  auto& myFunctions = ret["functions"];
  for (const auto& elem : theFunctions) {
    myFunctions.push_back(elem);
  }
  auto& myDependencies = ret["dependencies"];
  for (const auto& elem : theStates.dependencies()) {
    auto& myList = myDependencies[elem.first];
    for (const auto& myFunction : elem.second) {
      myList.push_back(myFunction);
    }
  }
  return ret.dump(2);
}

Chain exampleChain() {
  return Chain({"f1", "f2", "f1"},
               {
                   {
                       "s0",
                       {"f1"},
                   },
                   {
                       "s1",
                       {"f1", "f2"},
                   },
                   {
                       "s2",
                       {"f2"},
                   },
                   {
                       "s3",
                       {},
                   },
               });
};

} // namespace model
} // namespace edge
} // namespace uiiit
