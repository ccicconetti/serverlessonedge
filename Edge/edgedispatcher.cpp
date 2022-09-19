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

#include "edgedispatcher.h"

#include "Support/conf.h"
#include "edgemessages.h"
#include "ptimeestimator.h"
#include "ptimeestimatorfactory.h"

#include <glog/logging.h>

namespace uiiit {
namespace edge {

EdgeDispatcher::EdgeDispatcher(const std::string&   aLambdaEndpoint,
                               const std::string&   aCommandsEndpoint,
                               const std::string&   aControllerEndpoint,
                               const bool           aSecure,
                               const support::Conf& aProcessorConf,
                               const support::Conf& aPtimeEstimatorConf,
                               const support::Conf& aClientConf)
    : EdgeLambdaProcessor(aLambdaEndpoint,
                          aCommandsEndpoint,
                          aControllerEndpoint,
                          aSecure,
                          aProcessorConf,
                          aClientConf)
    , thePtimeEstimator(
          PtimeEstimatorFactory::make(aSecure, aPtimeEstimatorConf)) {
  // noop
}

std::vector<ForwardingTableInterface*> EdgeDispatcher::tables() {
  std::vector<ForwardingTableInterface*> myRet(1);
  myRet[0] = thePtimeEstimator.get();
  return myRet;
}

std::string EdgeDispatcher::destination(const rpc::LambdaRequest& aReq) {
  return (*thePtimeEstimator)(aReq);
}

void EdgeDispatcher::processSuccess(const rpc::LambdaRequest& aReq,
                                    const std::string&        aDestination,
                                    const LambdaResponse&     aRep,
                                    const double              aTime) {
  thePtimeEstimator->processSuccess(aReq, aDestination, aRep, aTime);
}

void EdgeDispatcher::processFailure(const rpc::LambdaRequest& aReq,
                                    const std::string&        aDestination) {
  thePtimeEstimator->processFailure(aReq, aDestination);
}

} // namespace edge
} // namespace uiiit
