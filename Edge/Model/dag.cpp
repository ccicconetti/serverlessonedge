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

#include "Edge/Model/dag.h"

#include "Support/tostring.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

namespace uiiit {
namespace edge {
namespace model {

Dag::Dag(const Successors&    aSuccessors,
         const FunctionNames& aFunctionNames,
         const Dependencies&  aDependencies)
    : theSuccessors(aSuccessors)
    , thePredecessors(makePredecessors(aSuccessors))
    , theFunctionNames(aFunctionNames)
    , theStates(aDependencies) {
  assert(thePredecessors.size() == theSuccessors.size());

  // consistency checks
  if (aSuccessors.size() != (aFunctionNames.size() - 1)) {
    throw std::runtime_error("Invalid size of successors and function names: " +
                             std::to_string(aSuccessors.size()) + " vs " +
                             std::to_string(aFunctionNames.size() - 1));
  }

  std::set<std::string> myFunctions;
  for (const auto& myFunction : aFunctionNames) {
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

Dag::~Dag() {
  // noop
}

bool Dag::operator==(const Dag& aOther) const {
  return theSuccessors == aOther.theSuccessors and
         /* thePredecessors == aOther.thePredecessors and */
         theFunctionNames == aOther.theFunctionNames and
         theStates == aOther.theStates;
}

std::string Dag::name() const {
  return ::toString(theFunctionNames, "-");
}

std::set<std::string> Dag::uniqueFunctions() const {
  std::set<std::string> ret;
  for (const auto& myFunction : theFunctionNames) {
    ret.insert(myFunction);
  }
  return ret;
}

const Dag::Successors& Dag::successors() const {
  return theSuccessors;
}

std::set<std::string> Dag::successorNames(const size_t aIndex) const {
  if (aIndex > theSuccessors.size()) {
    throw std::runtime_error(
        "Out of range function index: " + std::to_string(aIndex) + " > " +
        std::to_string(theSuccessors.size()));
  } else if (aIndex == theSuccessors.size()) {
    return std::set<std::string>();
  }
  return toNames<std::set<std::string>>(theSuccessors[aIndex]);
}

std::set<std::string> Dag::predecessorNames(const size_t aIndex) const {
  if (aIndex > thePredecessors.size()) {
    throw std::runtime_error(
        "Out of range function index: " + std::to_string(aIndex) + " > " +
        std::to_string(thePredecessors.size()));
  } else if (aIndex == 0) {
    return std::set<std::string>();
  }
  return toNames<std::set<std::string>>(thePredecessors[aIndex - 1]);
}

const Dag::FunctionNames& Dag::functionNames() const {
  return theFunctionNames;
}

const States& Dag::states() const {
  return theStates;
}

std::string Dag::entryFunctionName() const {
  return toName(0);
}

std::string Dag::toString() const {
  std::stringstream ret;
  ret << "{ ";
  for (size_t i = 0; i < theSuccessors.size(); i++) {
    if (i > 0) {
      ret << "; ";
    }
    ret << toName(i) << " -> "
        << ::toString(toNames<std::vector<std::string>>(theSuccessors[i]), ",");
  }
  ret << "}, " << theStates.toString();
  return ret.str();
}

Dag Dag::fromJson(const std::string& aJson) {
  const auto myJson = json::parse(aJson);
  Successors mySuccessors;
  if (myJson.find("successors") == myJson.end() or
      myJson.find("functionNames") == myJson.end()) {
    throw std::runtime_error("Invalid JSON content for a DAG");
  }
  for (const auto& elem : myJson["successors"]) {
    Successors::value_type myArray(elem);
    mySuccessors.emplace_back(myArray);
  }
  FunctionNames myFunctionNames(myJson["functionNames"]);
  const auto    myStates = States::fromJson(aJson);
  return Dag(mySuccessors, myFunctionNames, myStates.dependencies());
}

std::string Dag::toJson() const {
  json  ret;
  auto& mySuccessors = ret["successors"];
  for (const auto& elem : theSuccessors) {
    mySuccessors.push_back(json::array());
    for (const auto& inner : elem) {
      mySuccessors.back().push_back(inner);
    }
  }
  auto& myFunctionNames = ret["functionNames"];
  for (const auto& elem : theFunctionNames) {
    myFunctionNames.push_back(elem);
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

Dag::Predecessors Dag::makePredecessors(const Successors& aSuccessors) {
  Predecessors ret(aSuccessors.size());
  for (size_t i = 0; i < aSuccessors.size(); i++) {
    for (const auto& j : aSuccessors[i]) {
      if (j == 0 or (j - 1) >= ret.size()) {
        throw std::runtime_error("Invalid successor graph");
      }
      ret[j - 1].emplace_back(i);
    }
  }
  return ret;
}

std::string Dag::toName(const size_t aIndex) const {
  if (aIndex < theFunctionNames.size()) {
    return theFunctionNames[aIndex];
  }
  return std::string();
}

Dag exampleDag() {
  return Dag({{1, 2}, {3}, {3}},
             {"f0", "f1", "f2", "f2"},
             {
                 {
                     "s0",
                     {"f0"},
                 },
                 {
                     "s1",
                     {"f0", "f1"},
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
