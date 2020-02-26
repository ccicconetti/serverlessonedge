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

#include "forwardingtable.h"

#include "Edge/Entries/entryleastimpedance.h"
#include "Edge/Entries/entryrandom.h"
#include "Edge/Entries/entryroundrobin.h"
#include "forwardingtableexceptions.h"

#include <glog/logging.h>

#include <cassert>

namespace uiiit {
namespace edge {

ForwardingTable::ForwardingTable(const Type aType)
    : ForwardingTableInterface()
    , theType(aType)
    , theMutex()
    , theTable() {
  LOG(INFO) << "Created forwarding table of type " << toString(aType);
}

void ForwardingTable::change(const std::string& aLambda,
                             const std::string& aDest,
                             const float        aWeight,
                             const bool         aFinal) {
  if (aWeight < 0) {
    throw InvalidWeight(aWeight);
  }

  const std::lock_guard<std::mutex> myLock(theMutex);

  auto ret = theTable.emplace(aLambda, nullptr);
  if (ret.second) {
    if (theType == Type::Random) {
      ret.first->second.reset(new entries::EntryRandom());
    } else if (theType == Type::LeastImpedance) {
      ret.first->second.reset(new entries::EntryLeastImpedance());
    } else {
      assert(theType == Type::RoundRobin);
      ret.first->second.reset(new entries::EntryRoundRobin());
    }
  }

  assert(ret.first != theTable.end());
  assert(static_cast<bool>(ret.first->second));
  ret.first->second->change(aDest, aWeight, aFinal);

  VLOG(1) << "Changed the weight of destination " << aDest << " for lambda "
          << aLambda << " to " << aWeight << (aFinal ? " (F)" : "");
}

void ForwardingTable::change(const std::string& aLambda,
                             const std::string& aDest,
                             const float        aWeight) {
  if (aDest.empty()) {
    throw NoDestinations();
  }
  if (aWeight < 0) {
    throw InvalidWeight(aWeight);
  }

  const std::lock_guard<std::mutex> myLock(theMutex);

  const auto it = theTable.find(aLambda);
  if (it == theTable.end()) {
    throw NoDestinations();
  }

  it->second->change(aDest, aWeight);

  VLOG(1) << "Changed the weight of destination " << aDest << " for lambda "
          << aLambda << " to " << aWeight;
}

void ForwardingTable::multiply(const std::string& aLambda,
                               const std::string& aDest,
                               const float        aFactor) {
  if (aDest.empty()) {
    throw NoDestinations();
  }
  if (aFactor <= 0) {
    throw InvalidWeightFactor(aFactor);
  }

  const std::lock_guard<std::mutex> myLock(theMutex);

  const auto it = theTable.find(aLambda);
  if (it == theTable.end()) {
    throw NoDestinations();
  }

  const auto myWeight = it->second->weight(aDest);
  it->second->change(aDest, myWeight * aFactor);

  VLOG(1) << "Changed the weight of destination " << aDest << " for lambda "
          << aLambda << " to " << (myWeight * aFactor);
}

void ForwardingTable::remove(const std::string& aLambda,
                             const std::string& aDest) {
  const std::lock_guard<std::mutex> myLock(theMutex);

  auto it = theTable.find(aLambda);

  if (it != theTable.end()) {
    const auto myRemoved = it->second->remove(aDest);
    LOG_IF(INFO, myRemoved)
        << "Removed destination " << aDest << " for lambda " << aLambda;

    if (it->second->empty()) {
      LOG(INFO) << "Lambda " << aLambda << " now has no destinations";
      theTable.erase(it);
    }
  }
}

void ForwardingTable::remove(const std::string& aLambda) {
  const std::lock_guard<std::mutex> myLock(theMutex);

  const auto myErased = theTable.erase(aLambda);
  LOG_IF(INFO, myErased > 0)
      << "Removed all destinations for lambda " << aLambda;
}

std::string ForwardingTable::operator()(const std::string& aLambda) {
  const std::lock_guard<std::mutex> myLock(theMutex);

  const auto it = theTable.find(aLambda);

  if (it != theTable.end()) {
    return (*it->second)();
  }

  throw NoDestinations();
}

std::set<std::string> ForwardingTable::lambdas() const {
  const std::lock_guard<std::mutex> myLock(theMutex);

  std::set<std::string> myLambdas;
  for (const auto& myEntry : theTable) {
    myLambdas.insert(myEntry.first);
  }
  return myLambdas;
}

std::map<std::string, std::pair<float, bool>>
ForwardingTable::destinations(const std::string& aLambda) const {
  const std::lock_guard<std::mutex> myLock(theMutex);

  const auto it = theTable.find(aLambda);
  if (it == theTable.end()) {
    return std::map<std::string, std::pair<float, bool>>();
  }

  return it->second->destinations();
}

std::map<std::string, std::map<std::string, std::pair<float, bool>>>
ForwardingTable::fullTable() const {
  const std::lock_guard<std::mutex> myLock(theMutex);

  std::map<std::string, std::map<std::string, std::pair<float, bool>>> myRet;
  for (const auto& myEntry : theTable) {
    for (const auto& myDestination : myEntry.second->destinations()) {
      myRet[myEntry.first][myDestination.first] = myDestination.second;
    }
  }
  return myRet;
}

const std::string& toString(const ForwardingTable::Type aType) {
  static const std::map<ForwardingTable::Type, std::string> myValues({
      {ForwardingTable::Type::Random, "random"},
      {ForwardingTable::Type::LeastImpedance, "least-impedance"},
      {ForwardingTable::Type::RoundRobin, "round-robin"},
  });
  assert(myValues.find(aType) != myValues.end());
  return myValues.find(aType)->second;
}

ForwardingTable::Type forwardingTableTypeFromString(const std::string& aType) {
  if (aType == "random") {
    return ForwardingTable::Type::Random;
  } else if (aType == "least-impedance") {
    return ForwardingTable::Type::LeastImpedance;
  } else if (aType == "round-robin") {
    return ForwardingTable::Type::RoundRobin;
  }
  throw InvalidForwardingTableType(aType);
}

} // namespace edge
} // namespace uiiit
