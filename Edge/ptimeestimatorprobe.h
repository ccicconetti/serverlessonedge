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

#include "Edge/destinationtable.h"
#include "Edge/edgeclientpool.h"
#include "Edge/ptimeestimator.h"
#include "Support/saver.h"

namespace uiiit {

namespace rpc {
class LambdaRequest;
}

namespace edge {

/**
 * Class estimating the lambda execution time by polling all the possible
 * destinations (emulates a centralized approach).
 */
class PtimeEstimatorProbe final : public PtimeEstimator
{
  struct Descriptor {};

 public:
  explicit PtimeEstimatorProbe(const bool         aSecure,
                               const size_t       aMaxClients,
                               const std::string& aOutput);

  /**
   * \return the destination for the given lambda.
   *
   * \throw NoDestinations if the given lambda is not in the table.
   */
  std::string operator()(const rpc::LambdaRequest& aReq) override;

  /**
   * Simply save the actual vs. estimate time.
   */
  void processSuccess(const rpc::LambdaRequest& aReq,
                      const std::string&        aDestination,
                      const LambdaResponse&     aRep,
                      const double              aTime) override;

 private:
  void privateAdd(const std::string& aLambda,
                  const std::string& aDestination) override;
  void privateRemove(const std::string& aLambda,
                     const std::string& aDestination) override;

 private:
  EdgeClientPool               theClients;
  DestinationTable<Descriptor> theDestinations;
  support::Saver               theSaver;
};

} // namespace edge
} // namespace uiiit
