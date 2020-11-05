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

#include "edgerouter.h"

#include "Support/conf.h"
#include "edgemessages.h"
#include "localoptimizer.h"
#include "localoptimizerfactory.h"
#include "forwardingtablefactory.h"

namespace uiiit {
namespace edge {

EdgeRouter::EdgeRouter(const std::string&   aLambdaEndpoint,
                       const std::string&   aCommandsEndpoint,
                       const std::string&   aControllerEndpoint,
                       const size_t         aNumThreads,
                       const support::Conf& aProcessorConf,
                       const support::Conf& aTableConf,
                       const support::Conf& aLocalOptimizerConf)
    : EdgeLambdaProcessor(aLambdaEndpoint,
                          aCommandsEndpoint,
                          aControllerEndpoint,
                          aNumThreads,
                          aProcessorConf)
    , theOverallTable(ForwardingTableFactory::make(aTableConf))
    , theOverallOptimizer(
          LocalOptimizerFactory::make(*theOverallTable, aLocalOptimizerConf))
    , theFinalTable(ForwardingTableFactory::make(aTableConf))
    , theFinalOptimizer(
          LocalOptimizerFactory::make(*theFinalTable, aLocalOptimizerConf)) {
}

std::string EdgeRouter::destination(const rpc::LambdaRequest& aReq) {
  if (aReq.forward()) {
    return (*theFinalTable)(aReq.name());
  }
  return (*theOverallTable)(aReq.name());
}

void EdgeRouter::processSuccess(const rpc::LambdaRequest& aReq,
                                const std::string&        aDestination,
                                const LambdaResponse&     aRep,
                                const double              aTime) {
  std::ignore = aRep;
  assert(theOverallOptimizer);
  assert(theFinalOptimizer);

  // we keep optimization processes separated for the requests coming
  // from edge clients vs other nodes
  if (aReq.forward()) {
    (*theFinalOptimizer)(aReq, aDestination, aTime);
  } else {
    (*theOverallOptimizer)(aReq, aDestination, aTime);
  }
}

void EdgeRouter::processFailure(const rpc::LambdaRequest& aReq,
                                const std::string&        aDestination) {
  // regardless of whether the request came from an edge client or node
  // we remove the destination from both overall and final tables if
  // it is found to be unreachable
  (*theOverallTable).remove(aReq.name(), aDestination);
  (*theFinalTable).remove(aReq.name(), aDestination);
}

std::vector<ForwardingTableInterface*> EdgeRouter::tables() {
  std::vector<ForwardingTableInterface*> myRet(2);
  myRet[0] = theOverallTable.get();
  myRet[1] = theFinalTable.get();
  return myRet;
}

} // namespace edge
} // namespace uiiit
