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

#include "localoptimizertrivial.h"

#include "forwardingtable.h"

#include "edgeserver.grpc.pb.h"

#include <glog/logging.h>

#include <cassert>

namespace uiiit {
namespace edge {

LocalOptimizerTrivial::LocalOptimizerTrivial(ForwardingTable& aForwardingTable,
                                             const double     aPeriod,
                                             const Stat       aStat)
    : LocalOptimizerPeriodic(aForwardingTable, aPeriod)
    , theStat(aStat)
    , theMutex() {
  LOG(INFO) << "Creating a trivial local optimizer with update period "
            << aPeriod << " s, using stat " << toString(aStat);
}

void LocalOptimizerTrivial::operator()(const rpc::LambdaRequest& aReq,
                                       const std::string&        aDestination,
                                       const double              aTime) {
  VLOG(2) << "req " << aReq.name() << ", dest " << aDestination << ", time "
          << aTime << " s";

  const std::lock_guard<std::mutex> myLock(theMutex);
  theLatencies[aReq.name()][aDestination](aTime); // add entries if needed
}

void LocalOptimizerTrivial::update() {
  VLOG(2) << "Periodic update (start)";

  Latencies myLatencies;

  // grab the statistics so far, also cleaning the structure
  {
    const std::lock_guard<std::mutex> myLock(theMutex);
    myLatencies.swap(theLatencies);
  }

  // balance each entry
  for (const auto& myEntry : myLatencies) {
    balance(myEntry.first, myEntry.second);
  }

  VLOG(2) << "Periodic update (end)";
}

void LocalOptimizerTrivial::balance(const std::string&            aLambda,
                                    const Latencies::mapped_type& aEntry) {
  for (const auto& myElem : aEntry) {
    assert(myElem.second.count() > 0);
    double myLatency{0};
    switch (theStat) {
      case Stat::MEAN:
        myLatency = myElem.second.mean();
        break;
      case Stat::MIN:
        myLatency = myElem.second.min();
        break;
      case Stat::MAX:
        myLatency = myElem.second.max();
        break;
    }
    VLOG(2) << myElem.first << ", num " << myElem.second.count() << ", mean "
            << myElem.second.mean() << ", min " << myElem.second.min()
            << ", max " << myElem.second.max() << ", lat " << myLatency;
    assert(myLatency > 0.0);
    theForwardingTable.change(aLambda, myElem.first, myLatency);
  }
}

LocalOptimizerTrivial::Stat
LocalOptimizerTrivial::statFromString(const std::string& aStat) {
  if (aStat == "mean") {
    return Stat::MEAN;
  } else if (aStat == "min") {
    return Stat::MIN;
  } else if (aStat == "max") {
    return Stat::MAX;
  }
  throw std::runtime_error("Invalid stat: " + aStat);
}

std::string LocalOptimizerTrivial::toString(const Stat aStat) {
  switch (aStat) {
    case Stat::MEAN:
      return "mean";
    case Stat::MIN:
      return "min";
    case Stat::MAX:
      return "max";
  }
  throw std::runtime_error("Invalid stat found");
}

} // namespace edge
} // namespace uiiit
