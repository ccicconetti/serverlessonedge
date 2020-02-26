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

#pragma once

#include "forwardingtableexceptions.h"

#include <cassert>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <functional>

#ifndef NDEBUG
#include <glog/logging.h>
#include <list>
namespace details {
struct Printer {
    void operator()(const std::string& aDest, const float aValue) {
    theValues.push_back(std::make_pair(aDest, aValue));
  }
  void debugPrint() {
    if (not VLOG_IS_ON(2)) {
      return;
    }
    for (const auto& myElem : theValues) {
      LOG(INFO) << myElem.first << ": " << myElem.second;
    }
  }
  std::list<std::pair<std::string, float>> theValues;
};
} // namespace details
#else
namespace details {
struct Printer {
  void operator()(const std::string&, const float) {
  }
  void debugPrint() {
  }
};
} // namespace details
#endif

namespace uiiit {
namespace edge {

template <class TYPE>
class DestinationTable final
{
  using CtorFunc = std::function<std::unique_ptr<TYPE>(const std::string&,
                                                       const std::string&)>;
  using ObjFunc  = std::function<float(TYPE&)>;

 public:
  explicit DestinationTable(const CtorFunc& aCtorFunc)
      : theCtorFunc(aCtorFunc)
      , theDescriptors() {
  }

  /**
   * \return the descriptor for the given lambda and destination.
   *
   * \throw InvalidDestination if not found in the table.
   */
  TYPE& find(const std::string& aLambda, const std::string& aDestination);

  /**
   * \return the destination having the biggest objection function value and the
   * destination corresponding to that value, for a given lambda
   *
   * \throw NoDestinations if there are no destinations for the lambda
   */
  std::pair<std::string, float> best(const std::string& aLambda,
                                     const ObjFunc&     aObjFunc);

  /**
   * \return all possible values of the objection function for the given lambda.
   *
   * \throw NoDestinations if there are no destinations for the lambda
   */
  std::map<std::string, float> all(const std::string& aLambda,
                                   const ObjFunc&     aObjFunc);

  /**
   * Add a pair lambda, destination.
   *
   * \return true if an element was actually added, false otherwise.
   */
  bool add(const std::string& aLambda, const std::string& aDestination);

  /**
   * Remove a pair lambda, destination.
   *
   * \return true if an element was actually removed, false otherwise.
   */
  bool remove(const std::string& aLambda, const std::string& aDestination);

 private:
  // map of:
  // - key: lambda function name
  // - value: map of:
  //   - key: destination
  //   - value: unique pointer to the mapped TYPE
  using Descriptors =
      std::map<std::string, std::map<std::string, std::unique_ptr<TYPE>>>;

  const CtorFunc theCtorFunc;
  Descriptors    theDescriptors;
};

template <class TYPE>
TYPE& DestinationTable<TYPE>::find(const std::string& aLambda,
                                   const std::string& aDestination) {
  const auto it = theDescriptors.find(aLambda);
  if (it != theDescriptors.end()) {
    const auto jt = it->second.find(aDestination);
    if (jt != it->second.end()) {
      return *jt->second;
    }
  }
  throw InvalidDestination(aLambda, aDestination);
}

template <class TYPE>
std::pair<std::string, float>
DestinationTable<TYPE>::best(const std::string& aLambda,
                             const ObjFunc&     aObjFunc) {
  const auto it = theDescriptors.find(aLambda);
  if (it == theDescriptors.end()) {
    throw NoDestinations();
  }

  float            myMaxRtt = std::numeric_limits<float>::lowest();
  std::string      myMaxDest;
  details::Printer myPrinter;
  for (const auto& myDest : it->second) {
    const auto myCurRtt = aObjFunc(*myDest.second);
    myPrinter(myDest.first, myCurRtt);
    if (myCurRtt > myMaxRtt) {
      myMaxDest = myDest.first;
      myMaxRtt  = myCurRtt;
    }
  }
  myPrinter.debugPrint();
  assert(not myMaxDest.empty());
  return std::make_pair(myMaxDest, myMaxRtt);
}

template <class TYPE>
std::map<std::string, float>
DestinationTable<TYPE>::all(const std::string& aLambda,
                            const ObjFunc&     aObjFunc) {

  const auto it = theDescriptors.find(aLambda);
  if (it == theDescriptors.end()) {
    throw NoDestinations();
  }

  std::map<std::string, float> ret;
  for (const auto& myDest : it->second) {
    ret.emplace(myDest.first, aObjFunc(*myDest.second));
  }

  assert(not ret.empty());
  return ret;
}

template <class TYPE>
bool DestinationTable<TYPE>::add(const std::string& aLambda,
                                 const std::string& aDestination) {
  auto it = theDescriptors.find(aLambda);
  if (it == theDescriptors.end()) {
    const auto ret =
        theDescriptors.emplace(aLambda, typename Descriptors::mapped_type());
    assert(ret.second);
    it = ret.first;
  }

  assert(it != theDescriptors.end());

  auto jt = it->second.find(aDestination);
  if (jt == it->second.end()) {
    const auto ret = it->second.emplace(
        std::make_pair(aDestination, theCtorFunc(aLambda, aDestination)));
    assert(ret.second);
    jt = ret.first;

    return true;
  }

  assert(jt != it->second.end());
  assert(jt->second);

  return false;
}

template <class TYPE>
bool DestinationTable<TYPE>::remove(const std::string& aLambda,
                                    const std::string& aDestination) {
  auto it = theDescriptors.find(aLambda);
  if (it == theDescriptors.end()) {
    return false;
  }

  auto jt = it->second.find(aDestination);
  if (jt == it->second.end()) {
    return false;
  }

  it->second.erase(jt);
  if (it->second.empty()) {
    theDescriptors.erase(it);
  }

  return true;
}

} // namespace edge
} // namespace uiiit
