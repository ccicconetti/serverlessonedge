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

#include "Edge/edgemessages.h"
#include "Edge/forwardingtableinterface.h"
#include "Support/macros.h"

#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace uiiit {

namespace rpc {
class LambdaRequest;
}

namespace edge {

struct InvalidPtimeEstimatorType : public std::runtime_error {
  explicit InvalidPtimeEstimatorType(const std::string& aType)
      : std::runtime_error("Invalid processing time estimator type '" + aType +
                           "'") {
  }
};

/**
 * Class of objects returning an estimate of the processing time of a lambda.
 */
class PtimeEstimator : public ForwardingTableInterface
{
 protected:
  struct Estimates {
    float theRtt;
    float thePtime;
  };

 public:
  enum class Type : int {
    Test  = 0,
    Rtt   = 1,
    Util  = 2,
    Probe = 3,
  };

  NONCOPYABLE_NONMOVABLE(PtimeEstimator);

  //! Create a processing time estimator of the given type.
  explicit PtimeEstimator(const Type aType);

  /**
   * \return the destination for the given lambda.
   *
   * \throw NoDestinations if the given lambda is not in the table.
   */
  virtual std::string operator()(const rpc::LambdaRequest& aReq) = 0;

  /**
   * Notify that a lambda function has been correctly executed.
   *
   * \param aReq the lambda request.
   * \param aDestination the edge computer that executed the function.
   * \param aRep the lambda response.
   * \param aTime the overall execution time, including transport latency.
   */
  virtual void processSuccess(const rpc::LambdaRequest& aReq,
                              const std::string&        aDestination,
                              const LambdaResponse&     aRep,
                              const double              aTime) = 0;

  /**
   * Notify that a lambda function has failed.
   *
   * Automatically causes the destination to be removed from the set of possible
   * edge computers that can serve this lambda.
   */
  void processFailure(const rpc::LambdaRequest& aReq,
                      const std::string&        aDestination);

  /**
   * Add a new destination for a given lambda or change its weight.
   *
   * \param aLambda The name of the lambda function.
   *
   * \param aDest The destination to be added or whose weight has to be
   * changed.
   *
   * \param aWeight The destination weight. Unused.
   *
   * \param aFinal True if this is the final destination.
   */
  void change(const std::string& aLambda,
              const std::string& aDest,
              const float        aWeight,
              const bool         aFinal) override final;

  /**
   * Unused because PtimeEstimator does not allow changing weights.
   */
  void change(const std::string& aLambda,
              const std::string& aDest,
              const float        aWeight) override final;

  //! Remove a destination for a given lambda.
  void remove(const std::string& aLambda,
              const std::string& aDest) override final;

  //! Remove all destinations for a given lambda.
  void remove(const std::string& aLambda) override final;

  //! \return all possible lambda served.
  std::set<std::string> lambdas() const override final;

  //! \return a full representation of the table.
  std::map<std::string, std::map<std::string, std::pair<float, bool>>>
  fullTable() const override final;

 protected:
  //! \return the input size of the lambda request.
  static size_t size(const rpc::LambdaRequest& aReq);

 private:
  //! Internal function to remove a destination for a given lambda.
  void internalRemove(const std::string& aLambda, const std::string& aDest);

  //! Called as a new destination is added for the given lambda.
  virtual void privateAdd(const std::string& aLambda,
                          const std::string& aDestination) = 0;
  //! Called as an existing destination for a given lambda is removed.
  virtual void privateRemove(const std::string& aLambda,
                             const std::string& aDestination) = 0;

  void assertConsistency(const std::string& aLambda) const;

 protected:
  const Type            theType;
  mutable std::mutex    theMutex;
  std::set<std::string> theLambdas;
  std::map<std::string, std::map<std::string, std::pair<float, bool>>> theTable;
  std::unordered_map<uint64_t, Estimates> theEstimates;
};

const std::string& toString(const PtimeEstimator::Type aType);

PtimeEstimator::Type ptimeEstimatorTypeFromString(const std::string& aType);

} // namespace edge
} // namespace uiiit
