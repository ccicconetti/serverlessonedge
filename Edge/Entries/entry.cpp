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

#include "entry.h"

#include "Edge/forwardingtableexceptions.h"

#include <algorithm>

namespace uiiit {
namespace edge {
namespace entries {

Entry::Entry()
    : theDestinations() {
}

void Entry::change(const std::string& aDest,
                   const float        aWeight,
                   const bool         aFinal) {
  if (aDest.empty() or aWeight <= 0.0f) {
    throw InvalidDestination(aDest, aWeight);
  }

  auto it = std::find_if(
      theDestinations.begin(),
      theDestinations.end(),
      [&aDest](const auto& aElem) { return aElem.theDestination == aDest; });

  if (it != theDestinations.end()) {
    const auto myOldWeight = it->theWeight;
    it->theWeight          = aWeight;
    it->theFinal           = aFinal;

    updateWeight(aDest, myOldWeight, aWeight);

  } else {
    theDestinations.emplace_back(Element{aDest, aWeight, aFinal});
    updateAddDest(aDest, aWeight);
  }
}

void Entry::change(const std::string& aDest, const float aWeight) {
  if (aDest.empty()) {
    throw NoDestinations();
  }

  if (aWeight <= 0.0f) {
    throw InvalidDestination(aDest, aWeight);
  }

  auto it = std::find_if(
      theDestinations.begin(),
      theDestinations.end(),
      [&aDest](const auto& aElem) { return aElem.theDestination == aDest; });

  if (it == theDestinations.end()) {
    throw NoDestinations(aDest);
  }

  const auto myOldWeight = it->theWeight;
  it->theWeight          = aWeight;

  updateWeight(aDest, myOldWeight, aWeight);
}

float Entry::weight(const std::string& aDest) const {
  if (aDest.empty()) {
    throw NoDestinations();
  }

  auto it = std::find_if(
      theDestinations.begin(),
      theDestinations.end(),
      [&aDest](const auto& aElem) { return aElem.theDestination == aDest; });

  if (it == theDestinations.end()) {
    throw NoDestinations(aDest);
  }
  return it->theWeight;
}

bool Entry::remove(const std::string& aDest) {
  for (auto it = theDestinations.begin(); it != theDestinations.end(); ++it) {
    if (it->theDestination == aDest) {
      const auto myDestination = it->theDestination;
      const auto myWeight      = it->theWeight;
      theDestinations.erase(it);
      updateDelDest(myDestination, myWeight);
      return true;
    }
  }
  return false;
}

std::map<std::string, std::pair<float, bool>> Entry::destinations() const {
  std::map<std::string, std::pair<float, bool>> myDestinations;
  for (const auto& myElem : theDestinations) {
    myDestinations.emplace(myElem.theDestination,
                           std::make_pair(myElem.theWeight, myElem.theFinal));
  }
  return myDestinations;
}

} // namespace entries
} // namespace edge
} // namespace uiiit
