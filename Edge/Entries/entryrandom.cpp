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

#include "entryrandom.h"

#include "Edge/forwardingtableexceptions.h"
#include "Support/random.h"

#include <cassert>

namespace uiiit {
namespace edge {
namespace entries {

EntryRandom::EntryRandom()
    : Entry()
    , theInvSum(0) {
}

std::string EntryRandom::operator()() {
  if (theDestinations.empty()) {
    throw NoDestinations();
  }

  // if there is a single destination then we can skip the logic for selection
  if (theDestinations.size() == 1) {
    return theDestinations.front().theDestination;
  }

  const auto myRand = support::random() * theInvSum; // [0, theSum)

  float mySum{0.0f};
  for (auto it = theDestinations.begin();
       it != std::prev(theDestinations.end());
       ++it) {
    assert(it->theWeight != 0.0f);
    mySum += 1.0f / it->theWeight;
    if (myRand <= mySum) {
      return it->theDestination;
    }
  }

  return theDestinations.back().theDestination;
}

void EntryRandom::updateWeight(const std::string& aDest,
                               const float        aOldWeight,
                               const float        aNewWeight) {
  assert(aNewWeight != 0);
  assert(aOldWeight != 0);
  theInvSum += (1.0f / aNewWeight) - (1.0f / aOldWeight);
}

void EntryRandom::updateAddDest(const std::string& aDest, const float aWeight) {
  assert(aWeight != 0);
  theInvSum += 1.0f / aWeight;
}

void EntryRandom::updateDelDest(const std::string& aDest, const float aWeight) {
  assert(aWeight != 0);
  theInvSum -= 1.0f / aWeight;
}

} // namespace entries
} // namespace edge
} // namespace uiiit
