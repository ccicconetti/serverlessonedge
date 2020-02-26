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

#include "entryleastimpedance.h"

#include "Edge/forwardingtableexceptions.h"

#include <algorithm>
#include <cassert>

namespace uiiit {
namespace edge {
namespace entries {

EntryLeastImpedance::EntryLeastImpedance()
    : Entry()
    , theMinElement(theDestinations.end()) {
}

std::string EntryLeastImpedance::operator()() {
  if (theDestinations.empty()) {
    throw NoDestinations();
  }
  return theMinElement->theDestination;
}

void EntryLeastImpedance::updateWeight(const std::string& aDest,
                                       const float        aOldWeight,
                                       const float        aNewWeight) {
  assert(theMinElement != theDestinations.end());
  if (aNewWeight <= theMinElement->theWeight or
      aOldWeight == theMinElement->theWeight) {
    update();
  }
}

void EntryLeastImpedance::updateAddDest(const std::string& aDest,
                                        const float        aWeight) {
  if (theMinElement == theDestinations.end()) {
    theMinElement = theDestinations.begin();
  } else if (aWeight < theMinElement->theWeight) {
    update();
  }
}

void EntryLeastImpedance::updateDelDest(const std::string& aDest,
                                        const float        aWeight) {
  if (theMinElement->theWeight == aWeight) {
    update();
  }
}

void EntryLeastImpedance::update() {
  theMinElement =
      std::min_element(theDestinations.begin(), theDestinations.end());
}

} // namespace entries
} // namespace edge
} // namespace uiiit
