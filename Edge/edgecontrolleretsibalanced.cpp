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

#include "edgecontrolleretsibalanced.h"

#include "Support/tostring.h"

#include <glog/logging.h>

#include <algorithm>
#include <cassert>

namespace uiiit {
namespace edge {

EdgeControllerEtsiBalanced::EdgeControllerEtsiBalanced(
    const std::string& aEndpoint, const double aOptimizationPeriod)
    : EdgeController()
    , thePeriodicMutex()
    , theClient(aEndpoint)
    , theDesc()
    , theComputerEndpoints()
    , theLastTableReceived()
    , thePeriodicTask([this]() { optimize(); }, aOptimizationPeriod) {
  LOG_IF(WARNING, not theClient.table().empty())
      << "The edge router association table of the UE app LCM proxy is not "
         "empty as expected";
}

void EdgeControllerEtsiBalanced::addLambda(const std::string& aLambda) {
  const std::lock_guard<std::mutex> myLock(thePeriodicMutex);
  theClient.addLambda(aLambda);
}

void EdgeControllerEtsiBalanced::delLambda(const std::string& aLambda) {
  const std::lock_guard<std::mutex> myLock(thePeriodicMutex);
  theClient.delLambda(aLambda);
}

void EdgeControllerEtsiBalanced::addComputer(
    const std::string& aEndpoint, const std::list<std::string>& aLambdas) {
  const std::lock_guard<std::mutex> myLock(thePeriodicMutex);
  assert(not aEndpoint.empty());

  theComputerEndpoints.insert(aEndpoint);

  for (const auto& myLambda : aLambdas) {
    assert(not myLambda.empty());

    // the LCM proxy is only notified if aEndpoint is an edge computer for
    // myLambda and no other edge computers are currently serving it
    const auto it = theDesc.emplace(myLambda, Desc(aEndpoint));
    if (it.second) {
      theClient.associateAddress("", myLambda, aEndpoint);
    } else {
      assert(not it.first->second.theComputers.empty());
      const auto ret = it.first->second.theComputers.insert(aEndpoint);
      std::ignore    = ret;
      assert(ret.second);
    }
  }
}

void EdgeControllerEtsiBalanced::delComputer(
    const std::string& aEndpoint, const std::list<std::string>& aLambdas) {
  const std::lock_guard<std::mutex> myLock(thePeriodicMutex);
  assert(not aEndpoint.empty());

  theComputerEndpoints.erase(aEndpoint);

  for (const auto& myLambda : aLambdas) {
    assert(not myLambda.empty());
    auto it = theDesc.find(myLambda);
    assert(it != theDesc.end());
    if (it == theDesc.end()) {
      continue;
    }
    const auto myErased = it->second.theComputers.erase(aEndpoint);
    assert(myErased == 1);
    if (myErased == 0) {
      continue;
    }

    if (it->second.theDefault == aEndpoint) {
      if (it->second.theComputers.empty()) {
        // if the set of computers becomes empty, then we must remove the
        // descriptor, too: myLambda service will not available anymore
        theDesc.erase(it);
        theClient.removeAddress("", myLambda);

      } else {
        // otherwise, a new edge computer is notified as the default
        it->second.theDefault = *it->second.theComputers.begin();
        assert(not it->second.theDefault.empty());
        theClient.associateAddress("", myLambda, it->second.theDefault);
      }
    }
  }
}

void EdgeControllerEtsiBalanced::optimize() {
  const std::lock_guard<std::mutex> myLock(thePeriodicMutex);

  // retrieve the active clients and compute the new associations table
  auto myNewTable = theClient.contexts();
  if (myNewTable == theLastTableReceived) {
    VLOG(1) << "Received same table of active contexts as before: ignored";
    return;
  }
  theLastTableReceived.swap(myNewTable);
  LOG(INFO) << "Received a table of active contexts different from the last "
               "one: performing a new optimization cycle";

  // run optimization and enforce the new associations
  for (const auto& myRow : table(theLastTableReceived, theDesc)) {
    theClient.associateAddress(
        std::get<0>(myRow), std::get<1>(myRow), std::get<2>(myRow));
  }
}

EdgeControllerEtsiBalanced::Table
EdgeControllerEtsiBalanced::table(const Table& aTable, const DescMap& aDesc) {
  struct Counter {
    std::string theClient;  // edge client address
    std::string theLambda;  // lambda function name
    size_t      theCounter; // number of clients

    bool operator<(const Counter& aOther) const noexcept {
      // sort in descending order
      return theCounter > aOther.theCounter;
    }
  };

  struct Computer {
    explicit Computer(const std::string& aEndpoint)
        : theEndpoint(aEndpoint)
        , theLambdas()
        , theAssociations(0) {
    }

    std::string           theEndpoint;     // edge computer end-point
    std::set<std::string> theLambdas;      // set of lambda functions supported
    size_t                theAssociations; // number of associations

    bool operator<(const Computer& aOther) const noexcept {
      return theAssociations < aOther.theAssociations;
    }
  };

  // count the number of clients with a given lambda
  // key:   lambda function name
  // value: map of:
  //        key:   edge client address
  //        value: number of clients from that address using the given lambda
  std::map<std::string, std::map<std::string, size_t>> myNumClientsMap;
  for (const auto& elem : aTable) {
    myNumClientsMap[std::get<1>(elem)][std::get<0>(elem)]++;
  }

  // sort the above in a dedicated data structure
  // list of Counter elements, each containing
  // - the edge client address
  // - the lambda function name
  // - the number of clients from that address using the given lambda
  // sorted in descending order of counters of clients
  std::list<Counter> myNumClientsList;
  for (const auto& outer : myNumClientsMap) {
    assert(not outer.second.empty());
    for (const auto& inner : outer.second) {
      myNumClientsList.push_back(
          Counter{inner.first, outer.first, inner.second});
    }
  }
  myNumClientsList.sort();

  // initialize the counter of computer associations
  std::list<Computer> myComputers;
  for (const auto& elem : aDesc) {
    const auto& myLambda = elem.first;
    for (const auto& myEndpoint : elem.second.theComputers) {
      auto it = std::find_if(myComputers.begin(),
                             myComputers.end(),
                             [&myEndpoint](const Computer& aComputer) {
                               return aComputer.theEndpoint == myEndpoint;
                             });
      if (it == myComputers.end()) {
        it = myComputers.emplace(it, Computer(myEndpoint));
      }
      assert(it != myComputers.end());
      it->theLambdas.insert(myLambda);
    }
  }

  // assign each group of clients with the same address and using the same
  // lambda to the same edge computer by creating an end-point association
  Table myTable; // return value

  // fit each element in the least used computer that supports the lambda
  std::set<std::string> myUnserved;
  for (const auto& myCounter : myNumClientsList) {
    const auto& myLambda = myCounter.theLambda;
    auto        it       = std::find_if(myComputers.begin(),
                           myComputers.end(),
                           [&myLambda](const Computer& aComputer) {
                             return aComputer.theLambdas.count(myLambda) > 0;
                           });
    if (it == myComputers.end()) {
      // there is no computer serving that lambda
      myUnserved.insert(myLambda);
      continue;
    }

    // increase the number of associations by the number of clients
    // using the lambda from the current address
    auto myComputer = *it;
    myComputer.theAssociations += myCounter.theCounter;
    const auto myNumAssociations = myComputer.theAssociations;

    // remove the current element and insert it a bit later to maintain the
    // ascending order
    it = myComputers.erase(it);

    it = std::find_if(
        it, myComputers.end(), [myNumAssociations](const Computer& aComputer) {
          return aComputer.theAssociations >= myNumAssociations;
        });

    // add the association to the data structure returned below
    myTable.emplace_back(
        myCounter.theClient, myCounter.theLambda, myComputer.theEndpoint);

    // insert the updated computer structure into the sorted list
    myComputers.emplace(it, std::move(myComputer));
  }

  LOG_IF(WARNING, not myUnserved.empty())
      << "there is no computer serving the following lambdas: "
      << toString(myUnserved, ",");

  return myTable;
}

} // namespace edge
} // namespace uiiit
