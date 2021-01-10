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

#include "edgeclientpool.h"
#include "edgeserver.h"

#include <memory>
#include <vector>

namespace uiiit {

namespace support {
class Conf;
}

namespace edge {

class EdgeControllerClient;
class ForwardingTableInterface;

/**
 * Edge server that is capable of processing lambda requests via an
 * EdgeClientPool.
 *
 * The actual logic for deciding the destination must be implemented by derived
 * classes.
 */
class EdgeLambdaProcessor : public EdgeServer
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
   * \param aConf the configuration of this object.
   *
   * The configuration consists of the following parameters:
   *
   * - max-pending-clients=K
   *   Maximum number of client instances created to forward lambdas.
   *
   * - min-forward-time=A
   * - max-forward-time=B
   *   In addition to the real time of selecting the destination of any lambda,
   *   we add an artificial process time randomly drawn from U[A, B].
   *   A and B are in fractional seconds.
   */
  explicit EdgeLambdaProcessor(const std::string&   aLambdaEndpoint,
                               const std::string&   aCommandsEndpoint,
                               const std::string&   aControllerEndpoint,
                               const support::Conf& aRouterConf,
                               const bool           quicEnabled);

  ~EdgeLambdaProcessor() override;

  static std::string defaultConf();

  virtual std::vector<ForwardingTableInterface*> tables() = 0;

 private:
  //! \return the destination associated to the given lambda request.
  virtual std::string destination(const rpc::LambdaRequest& aReq) = 0;

  /**
   * Called upon successful execution of a lambda function on a computer.
   *
   * \param aReq the lambda request.
   *
   * \param aDestination the edge computer that executed the lambda.
   *
   * \param aRep the lambda response obtained from the edge computer.
   *
   * \param aTime the time required for the execution of the lambda.
   */
  virtual void processSuccess(const rpc::LambdaRequest& aReq,
                              const std::string&        aDestination,
                              const LambdaResponse&     aRep,
                              const double              aTime) = 0;

  /**
   * Called upon failed execution of a lambda function on a computer.
   *
   * \param aReq the lambda request that failed.
   *
   * \param aDestination the edge computer that failed to execute the lambda.
   */
  virtual void processFailure(const rpc::LambdaRequest& aReq,
                              const std::string&        aDestination) = 0;

  //! Perform actual processing of a lambda request.
  rpc::LambdaResponse process(const rpc::LambdaRequest& aReq) override;

  /**
   * If the end-point of a controller was specified in the ctor, announce this
   * element to it.
   */
  void init() override;

  //! Send a command to the controller.
  void controllerCommand(
      const std::function<void(EdgeControllerClient&)>& aCommand) noexcept;

  struct RandomWaiter {
    explicit RandomWaiter(const double aMin, const double aMax);
    void operator()() const;

    const double theMin;
    const double theSpan;
  };

 private:
  const std::string theCommandsEndpoint;
  const std::string theControllerEndpoint;
  const bool        theFakeProcessor;

  EdgeClientPool                        theClientPool;
  std::unique_ptr<EdgeControllerClient> theControllerClient;
  RandomWaiter                          theRandomWaiter;
};

} // end namespace edge
} // end namespace uiiit
