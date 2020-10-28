#pragma once

#include "localoptimizer.h"

#include <map>
#include <mutex>

namespace uiiit {
namespace edge {

  class LocalOptimizerFactory;

  /**
   * Optimizer which takes action to change the weight towards a
   * destination immediately based on the last latency reported.
   *
   * To implement Proportional Fairness scheduling algorithm are used
   * Prioritarization Coefficients (PC), these coefficients coincide with the
   * weight of a given destination for a given lambda. At each lambda request
   * the destination with the maximum PC is chosen.
   *
   * PCs are updated using the priority function formula:
   *
   * PC(t+1) = T^alpha / R^beta
   *
   * where:
   *
   * - T is an approximation of the istantaneous rate experienced using the
   * destination to serve the lambda request; it is computed as the inverse of
   * the last latency experienced to serve the LambdaRequest using this
   * destination
   * - R is the mean of the rates experienced in the past serving the
   * LambdaRequest using this destination (in order to keep this field updated
   * the struct Elem contains a counter and the sum of the inverse of the
   * "rates" experienced)
   * - alpha, beta are ctor parameters passed by CLI to tune the fairness of the
   * scheduling algorithm, in particular:
   *      + alpha = 0, beta = 1 => RoundRobin
   *      + alpha = 1, beta = 0 => max unfairness (always the best, maximizes
   * the throughput)
   */
  class LocalOptimizerAsyncPF final : public LocalOptimizer
  {
    friend class LocalOptimizerFactory;

   private:
    struct Elem {
      double thePrioritizationCoefficient;
      double theLambdaServedCount;
      double theExperiencedRateSum;
      // double theTimestamp; //TODO: Staleness Verification: are alpha, beta
      // sufficient to prevent staleness?
    };

    void operator()(const rpc::LambdaRequest& aReq,
                    const std::string&        aDestination,
                    const double              aTime) override;

    explicit LocalOptimizerAsyncPF(ForwardingTable& aForwardingTable,
                                 const double     aAlpha,
                                 const double     aBeta);

   private:
    // ctor arguments
    const double theAlpha;
    const double theBeta;

    // internal state
    std::mutex                                         theMutex;
    std::map<std::string, std::map<std::string, Elem>> theWeights;
  };

  } // namespace edge
} // namespace uiiit