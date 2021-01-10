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

#pragma once

#include "edgelambdaprocessor.h"

#include <memory>
#include <vector>

namespace uiiit {

namespace support {
class Conf;
}

namespace edge {

class ForwardingTableInterface;
class PtimeEstimator;

/**
 * Class implementing an edge dispatcher, ie. an application that dispatches
 * lambda function requests from edge clients towards edge computers.
 *
 * The possible destinations are known to the dispatcher from an external
 * entity, eg. an edge controller.
 *
 * The edge dispatcher currently supports a single forwarding table.
 */
class EdgeDispatcher final : public EdgeLambdaProcessor
{
 public:
  /**
   * \param aLambdaEndpoint the end-point to receive lambda requests.
   *
   * \param aCommandsEndpoint the end-point to receive commands from the
   * controller.
   *
   * \param aControllerEndpoint the end-point of the edge controller. Can be
   * empty, in which case it is assumed that there is no controller.
   *
   * \param aProcessorConf the configuration of the parent object.
   *
   * \param aPtimeEstimatorConf the configuration of the processing time
   * estimator object.
   */
  explicit EdgeDispatcher(const std::string&   aLambdaEndpoint,
                          const std::string&   aCommandsEndpoint,
                          const std::string&   aControllerEndpoint,
                          const support::Conf& aProcessorConf,
                          const support::Conf& aPtimeEstimatorConf,
                          const bool           aQuicEnabled);

  std::vector<ForwardingTableInterface*> tables() override;

 private:
  //! \return the destination associated to the given lambda request.
  std::string destination(const rpc::LambdaRequest& aReq) override;

  //! Called upon successful execution of a lambda function on a computer.
  void processSuccess(const rpc::LambdaRequest& aReq,
                      const std::string&        aDestination,
                      const LambdaResponse&     aRep,
                      const double              aTime) override;

  //! Called upon failed execution of a lambda function on a computer.
  void processFailure(const rpc::LambdaRequest& aReq,
                      const std::string&        aDestination) override;

 private:
  const std::shared_ptr<PtimeEstimator> thePtimeEstimator;
};

} // end namespace edge
} // end namespace uiiit
