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

#pragma once

#include "edgelambdaprocessor.h"
#include "forwardingtable.h"

#include <memory>
#include <vector>

namespace support {
class Conf;
}

namespace uiiit {
namespace edge {

class LocalOptimizer;

/**
 * Class implementing an edge router, i.e. an application that has a local
 * lambda forwarding table that is used to decide where to forward lambda
 * requests from edge clients. The forwarding table contains a  map of lambda
 * names to a list of weighted edge computer destinations.
 *
 * The policy to decide the destinations, if there are multiple choices for a
 * given lambda, is realized in the ForwardingTable object nested in the edge
 * router.
 *
 * The weights are updated by the LocalOptimizer object nested in the edge
 * router, based on the time required for the execution of past lambda functions
 * towards their respective destinations.
 *
 * The ForwardingTable and LocalOptimizer objects are configured according to
 * the values specified in the ctor of the EdgeRouter. Two instances each exist:
 * - overall: used for lambda requests coming from an edge client, which can be
 *   thus sent to any destination, including other edge nodes
 * - final: used for lambda requests that have been already forwarded by another
 *   edge node, hence they must be sent to a computer
 */
class EdgeRouter final : public EdgeLambdaProcessor
{
 public:
  /**
   * \param aLambdaEndpoint the end-point to receive Lambda request
   *
   * \param aCommandsEndpoint the end-point to receive commands from the
   * controller.
   *
   * \param aControllerEndpoint the end-point of the edge controller. Can be
   * empty, in which case it is assumed that there is no controller.
   *
   * \param aSecure if true then use SSL/TLS authentication.
   *
   * \param aProcessorConf the configuration of the parent object.
   *
   * \param aTableConf the configuration of the ForwardingTable object.
   *
   * \param aLocalOptimizerConf the configuration of the weight updater object.
   *
   * \param aClientConf the configuration of the clients used to forward lambda
   * requests.
   */
  explicit EdgeRouter(const std::string&   aLambdaEndpoint,
                      const std::string&   aCommandsEndpoint,
                      const std::string&   aControllerEndpoint,
                      const bool           aSecure,
                      const support::Conf& aProcessorConf,
                      const support::Conf& aTableConf,
                      const support::Conf& aLocalOptimizerConf,
                      const support::Conf& aClientConf);

  //! \return The forwarding tables: 0 is the overall, 1 is the final.
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
  std::unique_ptr<ForwardingTable> theOverallTable;
  std::shared_ptr<LocalOptimizer>  theOverallOptimizer;
  std::unique_ptr<ForwardingTable> theFinalTable;
  std::shared_ptr<LocalOptimizer>  theFinalOptimizer;
}; // namespace edge

} // namespace edge
} // end namespace uiiit
