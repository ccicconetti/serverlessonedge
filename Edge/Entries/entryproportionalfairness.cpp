#include "entryproportionalfairness.h"

#include "Edge/forwardingtableexceptions.h"

#include <algorithm>
#include <cassert>

namespace uiiit {
namespace edge {
namespace entries {

EntryProportionalFairness::EntryProportionalFairness()
    : Entry()
    , theMaxElement(theDestinations.end()) {
}

std::string EntryProportionalFairness::operator()() {
  if (theDestinations.empty()) {
    throw NoDestinations();
  }
  return theMaxElement->theDestination;
}


void EntryProportionalFairness::updateWeight(const std::string& aDest,
                                       const float        aOldWeight,
                                       const float        aNewWeight) {
  assert(theMaxElement != theDestinations.end());
  if (aNewWeight >= theMaxElement->theWeight or
      aOldWeight == theMaxElement->theWeight) {
    update();
  }
}

void EntryProportionalFairness::updateAddDest(const std::string& aDest,
                                        const float        aWeight) {
  if (theMaxElement == theDestinations.end()) {
    theMaxElement = theDestinations.begin();
  } else if (aWeight > theMaxElement->theWeight) {
    update();
  }
}

void EntryProportionalFairness::updateDelDest(const std::string& aDest,
                                        const float        aWeight) {
  if (theMaxElement->theWeight == aWeight) {
    update();
  }
}

void EntryProportionalFairness::update() {
  theMaxElement =
      std::max_element(theDestinations.begin(), theDestinations.end());
}

} // namespace entries
} // namespace edge
} // namespace uiiit
