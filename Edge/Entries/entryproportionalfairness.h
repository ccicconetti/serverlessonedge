#pragma once

#include "Support/chrono.h"
#include "entry.h"

#include <cmath>
#include <list>
#include <string>

namespace uiiit {
namespace edge {
namespace entries {

/**
 * Proportional Fairness Entry:
 * If the are multiple options available, the chosen entry must be the one with
 * maximum prioritization coefficient (computed in the functional operator each
 * time a destination for a given LambdaRequest must be scheduled)
 *
 * Prioritization coefficients PC are computed as: PC = T^alpha/R^beta where:
 * T = inverse of the last latency experienced serving a lambda request using
 * the destination
 * R = ratio between theLambdaServedCount and the difference between the
 * timestamp at scheduling time and theTimestamp associated with the
 * destination (the latter can be the timestamp in which the destination has
 * joined the system or the timestamp related to the last successful service of
 * a LambdaRequest given by the destination)
 * alpha,beta = are parametere obtained by CLI to tune the fairness of the
 * scheduler:
 *    - alpha = 0, beta = 1 => Round Robin
 *    - alpha = 1, beta = 0 => max unfairness, max throughput
 *    - alpha ~= 1, beta ~= 1 => 3G scheduling algorithm
 */
class EntryProportionalFairness final : public Entry
{
 public:
  explicit EntryProportionalFairness(double aAlpha, double aBeta);

 private:
  std::string operator()() override;

  void updateWeight(const std::string& aDest,
                    const float        aOldWeight,
                    const float        aNewWeight) override;
  void updateAddDest(const std::string& aDest, const float aWeight) override;
  void updateDelDest(const std::string& aDest, const float aWeight) override;

  float computeWeight(float  experiencedLatency,
                      int    theLambdaServedCount,
                      double theDestinationTimestamp,
                      double curTimestamp);

 private:
  support::Chrono theChrono;
  const double    theAlpha;
  const double    theBeta;
};

} // namespace entries
} // namespace edge
} // namespace uiiit