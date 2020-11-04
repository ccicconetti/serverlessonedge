#include "entryproportionalfairness.h"

#include "Edge/forwardingtableexceptions.h"
#include <algorithm>
#include <cassert>
#include <glog/logging.h>

namespace uiiit {
namespace edge {
namespace entries {

EntryProportionalFairness::EntryProportionalFairness(double aAlpha,
                                                     double aBeta)
    : Entry()
    , theChrono(true)
    , theAlpha(aAlpha)
    , theBeta(aBeta) {
}

/**
 * The Entry functional operator must compute all the prioritarization
 * coefficients and return the maximum one.
 */
std::string EntryProportionalFairness::operator()() {
  if (theDestinations.empty()) {
    throw NoDestinations();
  }

  std::string theScheduledDestination;
  auto        theMaxWeight = 0.f, itWeight = 0.f;
  const auto  myNow = theChrono.time();

  for (auto it = theDestinations.begin(); it != theDestinations.end(); ++it) {
    
    auto destinationStatsIt = thePFstats.find((*it).theDestination);
    assert(destinationStatsIt != thePFstats.end());

    /**
     * TODO: itWeight is write-only:
     * is it better to compute it twice or to compute it once and store it in
     * a variable?
     */
    itWeight = computeWeight((*it).theWeight,
                             (*destinationStatsIt).second.first,
                             (*destinationStatsIt).second.second,
                             myNow);
    if (itWeight > theMaxWeight) {
      theMaxWeight            = itWeight;
      theScheduledDestination = (*it).theDestination;
    }
  }
  return theScheduledDestination;
}

/**
 * experiencedLatency is the latency experienced and stored in theWeight field
 * by the LocalOptimizer during the processSuccess or 1  if the entry is "fresh"
 * (just inserted by the controller)
 */
float EntryProportionalFairness::computeWeight(float  experiencedLatency,
                                               int    theLambdaServedCount,
                                               double theDestinationTimestamp,
                                               double curTimestamp) {
  return pow((1 / experiencedLatency), theAlpha) /
         pow((theLambdaServedCount / (curTimestamp - theDestinationTimestamp)),
             theBeta);
}

/**
 * this function only updates thePFstats (theLambdaServedCount and the
 * Timestamp) of the destination which has served the LambdaRequest
 */
void EntryProportionalFairness::updateWeight(const std::string& aDest,
                                             const float        aOldWeight,
                                             const float        aNewWeight) {
  LOG(INFO) << "EntryPF.updateWeight" << '\n';
  std::ignore = aOldWeight;
  std::ignore = aNewWeight;

  const auto myNow = theChrono.time();
  auto       it    = thePFstats.find(aDest);

  assert(it != thePFstats.end());

  (*it).second.first += 1;     // theLambdaServedCount update
  (*it).second.second = myNow; // theTimestamp update
  printPFstats();
}

/**
 * this function only updates thePFstats data structure inserting a new
 * destination as key and a pair in which theLambdaServedCount is set to 1
 * otherwise a "division by 0" exception is thrown during the computation of its
 * weight in the functional operator (invoked by the EdgeRouter.destination).
 */
void EntryProportionalFairness::updateAddDest(const std::string& aDest,
                                              const float        aWeight) {
  LOG(INFO) << "EntryPF.updateAddDest" << '\n';
  std::ignore = aWeight;
  thePFstats.insert({aDest, std::make_pair(1, theChrono.time())});
  printPFstats();
}

/**
 * this function only updates thePFstats data structure deleting the detination
 * removed in theDestination data structure.
 */
void EntryProportionalFairness::updateDelDest(const std::string& aDest,
                                              const float        aWeight) {
  LOG(INFO) << "EntryPF.updateDelDest" << '\n';
  std::ignore = aWeight;
  auto it     = thePFstats.find(aDest);
  assert(it != thePFstats.end());
  thePFstats.erase(it);
  printPFstats();
}

} // namespace entries
} // namespace edge
} // namespace uiiit
