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

#include "Edge/ptimeestimator.h"
#include "Edge/rttestimator.h"
#include "Edge/utilestimator.h"
#include "Support/saver.h"

namespace uiiit {

namespace rpc {
class LambdaRequest;
}

namespace edge {

/**
 * Class estimating the processing time of a lambda based on both RTT and an
 * estimate of the current edge computer loads.
 */
class PtimeEstimatorUtil final : public PtimeEstimator
{
 public:
  /**
   * \param aRttWindowSize the number of samples to keep in the moving window to
   * estimate the RTT.
   *
   * \param aRttStalePeriod the time after which a sample in the moving window
   * to estimate the RTT is considered stale, in seconds.
   *
   * \param aLoadTimeout the time after which the load information is considered
   * stale, in seconds.
   *
   * \param aUtilWindowSize the number of samples to keep in the moving window
   * to estimate the variance of the load1 values.
   *
   * \param aOutput where to save the estimated vs. measured RTT and processing
   * time; disable if empty
   */
  explicit PtimeEstimatorUtil(const size_t       aRttWindowSize,
                              const double       aRttStalePeriod,
                              const double       aUtilLoadTimeout,
                              const size_t       aUtilWindowSize,
                              const std::string& aOutput);

  /**
   * \return the destination for the given lambda.
   *
   * \throw NoDestinations if the given lambda is not in the table.
   */
  std::string operator()(const rpc::LambdaRequest& aReq) override;

  /**
   * Compute the RTT as the overall execution time minus the processing time in
   * the lambda response and use it to updated the RTT estimator.
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
  RttEstimator   theRttEstimator;
  UtilEstimator  theUtilEstimator;
  support::Saver theSaver;
};

} // namespace edge
} // namespace uiiit
