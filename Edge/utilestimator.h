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

#include "Support/chrono.h"
#include "Support/histogram.h"
#include "Support/linearestimator.h"
#include "Support/macros.h"
#include "Support/movingvariance.h"
#include "destinationtable.h"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>

namespace uiiit {
namespace edge {

class UtilEstimator final
{
  NONCOPYABLE_NONMOVABLE(UtilEstimator);

  class LambdaDescriptor
  {
    NONCOPYABLE_NONMOVABLE(LambdaDescriptor);

   public:
    /**
     * \param aParent the class containing this one, required to retrieve the
     * computer loads.
     *
     * \param aDestination the computer serving this lambda function.
     */
    explicit LambdaDescriptor(UtilEstimator&     aParent,
                              const std::string& aDestination);

    //! Estimate the processing time, in s.
    float ptime(const size_t aInputSize);

    /**
     * Add a new measurement.
     *
     * \param aInputSize the size of the lambda request input.
     * \param aPtime the time required for processing the lambda.
     * \param aLoad the load reported.
     */
    void
    add(const size_t aInputSize, const float aPtime, const unsigned int aLoad);

   private:
    // ctor configuration
    UtilEstimator&    theParent;
    const std::string theDestination;

    // one linear estimator per input size
    std::map<size_t, std::unique_ptr<support::LinearEstimator>> theEstimators;
  };

  class ComputerDescriptor
  {
    NONCOPYABLE_NONMOVABLE(ComputerDescriptor);

   public:
    /**
     * \param aLoadTimeout the time, in s, after which a load1 info is stale.
     */
    explicit ComputerDescriptor(const double aLoadTimeout);

    //! All known lambdas that can be served by this computer.
    std::set<std::string> theLambdas;

    /**
     * As an effect of the call to this function, the load1 information is
     * reset to 0 if the timeout configured in the ctor expires.
     *
     * \return the last load1 value and the time elapsed between now and when a
     * load information was received.
     *
     * The load is guaranteed to be between 0 and 99.
     * The delta time is guaranteed to be positive.
     */
    std::pair<unsigned int, double> lastLoad();

    /**
     * Add a new measurement.
     *
     * \param aLoad1 the 1-second average load reported.
     */
    void add(const unsigned int aLoad1);

   private:
    // to reset the load1 information when stale
    const double theLoadTimeout;

    // keep track of the last time we have received a load1 indication
    double          theLastMeas;
    unsigned int    theLastLoad1;
    support::Chrono theChrono;
  };

 public:
  /**
   * \param aLoadTimeout the time, in s, after which a load1 info is stale.
   *
   * \param aUtilWindowSize the number of samples to keep in the moving window
   * to estimate the variance of the load1 values.
   */
  explicit UtilEstimator(const double aLoadTimeout,
                         const size_t aUtilWindowSize);

  ~UtilEstimator();

  enum BestTuple {
    BT_DEST  = 0,
    BT_RTT   = 1,
    BT_PTIME = 2,
  };

  /**
   * Find the best destination for the given lambda.
   *
   * \param aLambda the lambda function name.
   * \param aInputSize the lambda input size.
   * \param aRtts the estimated RTTs for each possible destination.
   *
   * \return a tuple containing: the destination selected, the estimated RTT,
   * the estimated processing time.
   */
  std::tuple<std::string, float, float>
  best(const std::string&                  aLambda,
       const size_t                        aInputSize,
       const std::map<std::string, float>& aRtts);

  /**
   * \param aInputSize the size of the input lambda function
   * \return the destination with smallest processing time
   */
  std::pair<std::string, float> smallestPtime(const std::string& aLambda,
                                              const size_t       aInputSize);

  /**
   * Add a new measurement.
   *
   * \param aLambda the lambda function name.
   * \param aDestination the edge computer that processed the lambda.
   * \param aInputSize the lambda input size.
   * \param aPtime the measured processing time.
   * \param aLoad1 the reported 1-second average load.
   * \param aLoad10 the reported 10-second average load.
   */
  void add(const std::string& aLambda,
           const std::string& aDestination,
           const size_t       aInputSize,
           const float        aPtime,
           const unsigned int aLoad1,
           const unsigned int aLoad10);

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
  // const ctor configuration
  const double theLoadTimeout;
  const size_t theWindowSize;

  // internal state
  std::map<std::string, std::unique_ptr<ComputerDescriptor>>
                                     theComputers; // key: dest
  DestinationTable<LambdaDescriptor> theTable;     // key: dest, lambda
};

} // namespace edge
} // namespace uiiit
