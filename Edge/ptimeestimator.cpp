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

#include "ptimeestimator.h"

#include <glog/logging.h>

#include <cassert>

namespace uiiit {
namespace edge {

PtimeEstimator::PtimeEstimator(const Type aType)
    : ForwardingTableInterface()
    , theType(aType)
    , theMutex()
    , theLambdas()
    , theTable()
    , theEstimates() {
  LOG(INFO) << "Created a processing time estimator of type "
            << toString(aType);
}

void PtimeEstimator::change(const std::string& aLambda,
                            const std::string& aDest,
                            const float        aWeight,
                            const bool         aFinal) {
  std::ignore = aWeight;

  const std::lock_guard<std::mutex> myLock(theMutex);

  bool myAdded = false;
  auto it      = theTable.emplace(aLambda,
                             std::map<std::string, std::pair<float, bool>>(
                                 {{aDest, std::make_pair(1.0f, aFinal)}}));
  if (it.second) {
    theLambdas.emplace(aLambda);
    LOG(INFO) << "New lambda supported: " << aLambda << ", destination is "
              << aDest << (aFinal ? " (F)" : "");
    myAdded = true;

  } else {
    assert(it.first != theTable.end());
    const auto jt =
        it.first->second.emplace(aDest, std::make_pair(1.0f, aFinal));
    myAdded = jt.second;
    LOG_IF(INFO, myAdded) << "New destination added to lambda " << aLambda
                          << ": " << aDest << (aFinal ? " (F)" : "");
  }

  if (myAdded) {
    privateAdd(aLambda, aDest);
  }
}

void PtimeEstimator::change(const std::string& aLambda,
                            const std::string& aDest,
                            const float        aWeight) {
  std::ignore = aLambda;
  std::ignore = aDest;
  std::ignore = aWeight;
}

void PtimeEstimator::processFailure(const rpc::LambdaRequest& aReq,
                                    const std::string&        aDestination) {
  const std::lock_guard<std::mutex> myLock(theMutex);

  // remove the pending estimates from the data structure
  const auto myRemoved = theEstimates.erase(reinterpret_cast<uint64_t>(&aReq));
  std::ignore          = myRemoved; // ifdef NDEBUG
  assert(myRemoved == 1);

  // remove the destination from the edge computer table
  internalRemove(aReq.name(), aDestination);
}

void PtimeEstimator::remove(const std::string& aLambda,
                            const std::string& aDest) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  internalRemove(aLambda, aDest);
}

void PtimeEstimator::internalRemove(const std::string& aLambda,
                                    const std::string& aDest) {
  ASSERT_IS_LOCKED(theMutex);
  assertConsistency(aLambda);

  auto it = theTable.find(aLambda);

  if (it != theTable.end()) {
    const auto myRemoved = it->second.erase(aDest);
    if (myRemoved) {
      LOG(INFO) << "Removed destination " << aDest << " for lambda " << aLambda;
      privateRemove(aLambda, aDest);
    }

    if (it->second.empty()) {
      LOG(INFO) << "Lambda " << aLambda << " now has no destinations";
      theTable.erase(it);
      theLambdas.erase(aLambda);
    }
  }
}

void PtimeEstimator::remove(const std::string& aLambda) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  assertConsistency(aLambda);

  const auto it = theTable.find(aLambda);
  if (it != theTable.end()) {
    for (const auto& myElem : it->second) {
      privateRemove(aLambda, myElem.first);
    }
  }

  theLambdas.erase(aLambda);
  const auto myErased = theTable.erase(aLambda);
  LOG_IF(INFO, myErased > 0)
      << "Removed all destinations for lambda " << aLambda;
}

std::set<std::string> PtimeEstimator::lambdas() const {
  const std::lock_guard<std::mutex> myLock(theMutex);
  return theLambdas;
}

std::map<std::string, std::map<std::string, std::pair<float, bool>>>
PtimeEstimator::fullTable() const {
  const std::lock_guard<std::mutex> myLock(theMutex);
  return theTable;
}

const std::string& toString(const PtimeEstimator::Type aType) {
  static const std::map<PtimeEstimator::Type, std::string> myValues({
      {PtimeEstimator::Type::Test, "test"},
      {PtimeEstimator::Type::Rtt, "rtt"},
      {PtimeEstimator::Type::Util, "util"},
      {PtimeEstimator::Type::Probe, "probe"},
  });
  assert(myValues.find(aType) != myValues.end());
  return myValues.find(aType)->second;
}

PtimeEstimator::Type ptimeEstimatorTypeFromString(const std::string& aType) {
  if (aType == "test") {
    return PtimeEstimator::Type::Test;
  } else if (aType == "rtt") {
    return PtimeEstimator::Type::Rtt;
  } else if (aType == "util") {
    return PtimeEstimator::Type::Util;
  } else if (aType == "probe") {
    return PtimeEstimator::Type::Probe;
  }
  throw InvalidPtimeEstimatorType(aType);
}

size_t PtimeEstimator::size(const rpc::LambdaRequest& aReq) {
  return std::max(aReq.datain().size(), aReq.input().size());
}

void PtimeEstimator::assertConsistency(const std::string& aLambda) const {
  [[maybe_unused]] const auto myEmptyLambdas =
      theLambdas.find(aLambda) == theLambdas.end();
  [[maybe_unused]] const auto myEmptyTable =
      theTable.find(aLambda) == theTable.end();
  assert(myEmptyLambdas == myEmptyTable);
}

} // namespace edge
} // namespace uiiit
