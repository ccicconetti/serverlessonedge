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

#include "Support/linearestimator.h"
#include "Support/macros.h"
#include "destinationtable.h"

#include <string>

namespace uiiit {
namespace edge {

/**
 * Estimates the RTT of a lambda request based on the past execution times.
 *
 * The RTT is assumed to be the difference between the overall lambda execution
 * and the processing time, as reported in the lambda response.
 */
class RttEstimator final
{
  NONCOPYABLE_NONMOVABLE(RttEstimator);

  class Descriptor
  {
    NONCOPYABLE_NONMOVABLE(Descriptor);

   public:
    explicit Descriptor(const size_t aWindowSize, const double aStalePeriod);

    /**
     * \return the estimated RRT according to the current fit (never negative);
     * if there is not a valid fit, then return 0.
     */
    float rtt(const size_t aInputSize);

    /**
     * Add a new point to the window.
     *
     * \param aInputSize the lambda request size.
     * \param aRtt the measured RTT.
     */
    void add(const size_t aInputSize, const float aRtt);

   private:
    support::LinearEstimator theEstimator;
  };

 public:
  /**
   * \param aWindowSize the number of samples to keep in the window for the
   * estimation of the RTT for each pair lambda,destination.
   *
   * \param aStalePeriod the period after which samples are considered stale.
   */
  explicit RttEstimator(const size_t aWindowSize, const double aStalePeriod);

  ~RttEstimator();

  /**
   * Estimate the RTT for a given lambda if executed by a given computer.
   *
   * \param aLambda the lambda function name.
   * \param aDestination the edge computer.
   * \param aInputSize the size of the input of the lambda function
   *
   * \return the estimated RTT, in fractional seconds, or 0 if there are no
   * sufficient data for the given pair lambda, destination.
   */
  float rtt(const std::string& aLambda,
            const std::string& aDestination,
            const size_t       aInputSize);

  /**
   * Estimate RTT for a given lambda of a given size for all possible computers.
   *
   * \throw NoDestinations if the given lambda cannot be served by any computer.
   */
  std::map<std::string, float> rtts(const std::string& aLambda,
                                    const size_t       aInputSize);

  /**
   * \return the destination with shortest RTT for a given lambda and size and
   * the estimate RTT, in seconds.
   *
   * \throw NoDestinations if the given lambda cannot be served by any computer.
   */
  std::pair<std::string, float> shortestRtt(const std::string& aLambda,
                                            const size_t       aInputSize);

  //! Add a measurement.
  void add(const std::string& aLambda,
           const std::string& aDestination,
           const size_t       aInputSize,
           const float        aRtt);

  /**
   * Add a pair lambda, destination.
   *
   * \return true if an element was actually added, false otherwise.
   */
  bool add(const std::string& aLambda, const std::string& aDestination);

  /**
   * Remove a pair lambda, destination.
   *
   * \return true if an element was actually removed, false otherwise.
   */
  bool remove(const std::string& aLambda, const std::string& aDestination);

 private:
  DestinationTable<Descriptor> theTable;
};

} // namespace edge
} // namespace uiiit
