#pragma once

#include "entry.h"

#include <list>
#include <string>

namespace uiiit {
namespace edge {
namespace entries {

/**
 * Proportional Fairness Entry:
 * If the are multiple options available, the vhosen entry must be the one with
 * maximum prioritization coefficient (stored in the field weight in the Element
 * struct in Entry) Prioritization coefficients pc are computed as:
 * pc = T^alpha/R^beta
 * where:
 * T = inverse of the last latency experienced serving a lambda request using
 * the destination
 * R = historical rate within a period determined by the PeriodicLocalOptimizer
 * (mean of the inverses of the latencies experienced within the period)
 * alpha,beta = are parametere obtained by CLI to tune the fairness of the
 * scheduler:
 *    - alpha = 0, beta = 1 => Round Robin
 *    - alpha = 1, beta = 0 => max unfairness, max throughput
 *    - alpha ~= 1, beta ~= 1 => 3G scheduling algorithm
 */
class EntryProportionalFairness final : public Entry
{
 public:
  explicit EntryProportionalFairness();

 private:
  std::string operator()() override;

  void updateWeight(const std::string& aDest,
                    const float        aOldWeight,
                    const float        aNewWeight) override;
  void updateAddDest(const std::string& aDest, const float aWeight) override;
  void updateDelDest(const std::string& aDest, const float aWeight) override;

  void update();

 private:
  std::list<Element>::iterator theMaxElement;
};

} // namespace entries
} // namespace edge
} // namespace uiiit