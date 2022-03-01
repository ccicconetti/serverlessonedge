/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2022 C. Cicconetti <https://ccicconetti.github.io/>

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

#include "LambdaMuSim/scenario.h"

#include "StateSim/network.h"
#include "StateSim/node.h"

#include <glog/logging.h>

#include <cassert>
#include <openssl/ossl_typ.h>

namespace uiiit {
namespace lambdamusim {

Scenario::Scenario(
    const statesim::Network&                                 aNetwork,
    const double                                             aAlpha,
    const double                                             aBeta,
    const std::function<std::size_t(const statesim::Node&)>& aNumContainers,
    const std::function<double(const statesim::Node&)>&      aContainerCapacity,
    const std::size_t                                        aSeed)
    : theAlpha(aAlpha)
    , theBeta(aBeta)
    , theSeed(aSeed) {
  VLOG(1) << "created scenario with alpha " << theAlpha << ", beta " << theBeta
          << ", seed " << theSeed;

  // load brokers: all the network clients
  std::vector<statesim::Node*> myBrokerPtrs;
  for (const auto& myClient : aNetwork.clients()) {
    myBrokerPtrs.emplace_back(myClient);
    theBrokers.emplace_back(Broker());
  }

  // load edge nodes: all the network processing nodes except clients
  for (const auto& myProcessing : aNetwork.processing()) {
    if (std::find_if(myBrokerPtrs.begin(),
                     myBrokerPtrs.end(),
                     [myProcessing](const auto& aValue) {
                       return aValue == myProcessing;
                     }) == myBrokerPtrs.end()) {
      theEdges.emplace_back(Edge{aNumContainers(*myProcessing),
                                 aContainerCapacity(*myProcessing)});
    }
  }
}

double& Scenario::networkCost(const ID aBroker, const ID aEdge) noexcept {
  const auto myIndex = aBroker * theEdges.size() + aEdge;
  assert(myIndex < theNetworkCost.size());
  return theNetworkCost[myIndex];
}

} // namespace lambdamusim
} // namespace uiiit
