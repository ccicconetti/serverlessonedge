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

#include "entryroundrobin.h"

#include "Edge/forwardingtableexceptions.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <sstream>

#include <glog/logging.h>

namespace uiiit {
namespace edge {
namespace entries {

EntryRoundRobin::EntryRoundRobin()
    : theCache()
    , theQueue()
    , theChrono(true)
    , theStat() {
}

std::string EntryRoundRobin::operator()() {
  if (theQueue.empty()) {
    assert(theDestinations.empty());
    throw NoDestinations();
  }

  // save the current destination for return, mark timestamp and update deficit
  auto myElem = theQueue.top();
  theQueue.pop();
  const auto myDestination = myElem.theElem->first;
  myElem.theElem->second.theDeficit += myElem.theElem->second.theWeight;
  theQueue.push(myElem);

  return myDestination;
}

bool EntryRoundRobin::good(const double aWeight) {
  assert(theStat.count() > 0);
  if (theStat.count() == 1 or aWeight <= (theStat.min() * 2)) {
    return true;
  }
  return false;
}

void EntryRoundRobin::updateWeight(const std::string& aDest,
                                   const float        aOldWeight,
                                   const float        aNewWeight) {
  std::ignore = aOldWeight;
  updateWeight(aDest, aNewWeight);
}

void EntryRoundRobin::updateAddDest(const std::string& aDest,
                                    const float        aWeight) {
  std::ignore = aDest;
  std::ignore = aWeight;
  updateList();
}

void EntryRoundRobin::updateDelDest(const std::string& aDest,
                                    const float        aWeight) {
  std::ignore = aDest;
  std::ignore = aWeight;
  updateList();
}

void EntryRoundRobin::updateWeight(const std::string& aDest,
                                   const float        aWeight) {
  auto it = theCache.find(aDest);
  assert(it != theCache.end());
  it->second.theWeight      = aWeight;
  it->second.theLastUpdated = theChrono.time();

  // force the element with the smallest deficit to have 0 deficit
  const auto myMinDeficit = minDeficit();
  for (auto& myElem : theCache) {
    myElem.second.theDeficit -= myMinDeficit;
  }

  // update the active set
  updateActiveSet();

  debugPrintActiveSet();
}

float EntryRoundRobin::minDeficit() {
  float myMinDeficit = std::numeric_limits<float>::max();
  auto  myDup        = theQueue;
  while (not myDup.empty()) {
    const auto myElem = myDup.top();
    myDup.pop();
    myMinDeficit = std::min(myMinDeficit, myElem.theElem->second.theDeficit);
  }
  return myMinDeficit;
}

void EntryRoundRobin::updateList() {
  if (theDestinations.empty()) {
    theCache.clear();
    std::priority_queue<QueueElement> myEmptyQueue;
    theQueue.swap(myEmptyQueue);
    return;
  }

  // mark all cache elements to be removed, then uncheck those that are still
  // there also, add new entries to the cache
  for (auto& myPair : theCache) {
    myPair.second.theRemoved = true;
  }

  const auto myMinDeficit = minDeficit();
  for (const auto& myDestination : theDestinations) {
    const auto ret =
        theCache.insert({myDestination.theDestination,
                         CacheElement(myDestination.theWeight, myMinDeficit)});
    if (not ret.second) {
      // update weight and mark as non-vanishing
      ret.first->second.theRemoved = false;
      ret.first->second.theWeight  = myDestination.theWeight;
    }
  }

  // remove all vanishing elements
  for (auto it = theCache.begin(); it != theCache.end();) {
    if (it->second.theRemoved) {
      it = theCache.erase(it);
    } else {
      ++it;
    }
  }

  // update the active set
  updateActiveSet();

  debugPrintActiveSet();
}

void EntryRoundRobin::updateStat() {
  theStat.reset();
  for (const auto& elem : theCache) {
    if (elem.second.theRemoved) {
      continue;
    }
    theStat(elem.second.theWeight);
  }
  assert(theStat.count() > 0);
}

void EntryRoundRobin::updateActiveSet() {
  // compute the mean and standard deviation of the weights
  updateStat();

  std::priority_queue<QueueElement> myNewQueue;
  const auto                        myNow        = theChrono.time();
  const auto                        myMinDeficit = minDeficit();
  for (auto it = theCache.begin(); it != theCache.end(); ++it) {
    const auto myLastUpdated = it->second.theLastUpdated;
    auto       myActive      = false;

    if (good(it->second.theWeight)) {
      // the destination is admitted to the active set
      // if it was under probing then its stale period duration is reset to the
      // initial minimum value
      if (it->second.theProbing) {
        it->second.theProbing = false;
        it->second.resetStalePeriod();
      }
      myActive = true;

    } else if (myLastUpdated < 0) {
      // the destination is either brand new or its stale timer has expired:
      // we keep it in the active list waiting for its fate to be decided
      // after it has been used
      myActive = true;

    } else {
      // the destination has been recently used (myLastUpdated >= 0) but its
      // weight is not good enough to make it to the active set: if the
      // destination was under probing then we increase the stale period
      // duration
      if (it->second.theProbing) {
        it->second.theProbing = false;
        it->second.updateStalePeriod();
      }

      if ((myNow - myLastUpdated) >= it->second.theStalePeriod) {
        // stale timer expired: mark the destination as probing
        it->second.theLastUpdated = -1.0;
        it->second.theDeficit     = myMinDeficit;
        it->second.theProbing     = true;
        myActive                  = true;
      }
    }

    if (myActive) {
      // add the element to the active set
      myNewQueue.push(QueueElement{it});
    }
  }
  theQueue.swap(myNewQueue);
}

void EntryRoundRobin::debugPrintActiveSet() {
  if (VLOG_IS_ON(2)) {
    std::stringstream myStream;
    const auto        myNow = theChrono.time();
    for (auto it = theCache.begin(); it != theCache.end(); ++it) {
      myStream << '\n'
               << it->first << " weight " << it->second.theWeight
               << " last-updated " << it->second.theLastUpdated << " deficit "
               << it->second.theDeficit << " stale-period "
               << it->second.theStalePeriod << " ("
               << (it->second.theLastUpdated >= 0 ?
                       (it->second.theStalePeriod -
                        (myNow - it->second.theLastUpdated)) :
                       -1)
               << " remaining)" << (it->second.theRemoved ? " R" : "")
               << (it->second.theProbing ? " P" : "");
    }
    LOG(INFO) << "destinations " << theCache.size() << ", active set ("
              << theQueue.size() << "), current "
              << theQueue.top().theElem->first << myStream.str();
    auto myDup = theQueue;
    while (not myDup.empty()) {
      const auto myElem = myDup.top();
      myDup.pop();
      LOG(INFO) << myElem.theElem->first << ' '
                << myElem.theElem->second.theDeficit;
    }
  }
}

} // namespace entries
} // namespace edge
} // namespace uiiit
