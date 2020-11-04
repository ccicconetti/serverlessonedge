#pragma once

#include "Support/chrono.h"
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
 * A destination new weight is updated using the priority function formula:
 *
 *                     Weight(t+1) = T^alpha / R^beta
 *
 * where:
 * - T is an approximation of the istantaneous rate experienced using the
 * destination to serve the lambda request; it is computed as the inverse of
 * the last latency experienced to serve the LambdaRequest using this
 * destination:
 *                        T(t) = 1 / Latency(t)
 *
 * - R is an approximation of the historical rate experienced by the
 * destination, it is computed as the ratio between the number of Lambda served
 * and the difference between the timestamp in which the destination has entered
 * into the system and the one taken after it has served the last request.
 *
 * ->-> These two quantities must be refreshed after a stale period
 *
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
  void operator()(const rpc::LambdaRequest& aReq,
                  const std::string&        aDestination,
                  const double              aTime) override;

  explicit LocalOptimizerAsyncPF(ForwardingTable& aForwardingTable);

};

} // namespace edge
} // namespace uiiit