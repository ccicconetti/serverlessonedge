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

#include "Edge/Entries/entry.h"
#include "Edge/forwardingtableinterface.h"
#include "Support/macros.h"

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>

namespace uiiit {
namespace edge {

/**
 * Thread-safe table returning an end-point associated to a given lambda
 * function name.
 */
class ForwardingTable final : public ForwardingTableInterface
{
 public:
  enum class Type : int {
    Random         = 0,
    LeastImpedance = 1,
    RoundRobin     = 2,
    ProportionalFairness = 3,
  };

  NONCOPYABLE_NONMOVABLE(ForwardingTable);

  //! Create a forwarding table with no entries.
  explicit ForwardingTable(const Type aType);

  /**
   * Add a new destination for a given lambda or change its weight.
   *
   * \param aLambda The name of the lambda function.
   *
   * \param aDest The destination to be added or whose weight has to be
   * changed.
   *
   * \param aWeight The destination weight.
   *
   * \param aFinal True if this is the final destination.
   *
   * \throw InvalidDestination if the destination is empty.
   *
   * \throw InvalidWeight if the weight is non-positive.
   */
  void change(const std::string& aLambda,
              const std::string& aDest,
              const float        aWeight,
              const bool         aFinal) override;

  /**
   * Change the weight of an existing lambda.
   *
   * \param aLambda The name of the lambda function.
   *
   * \param aDest The destination to be added or whose weight has to be
   * changed.
   *
   * \param aWeight The destination weight.
   *
   * \throw NoDestinations if the destination is empty or not found.
   *
   * \throw InvalidWeightFactor if the weight factor is non-positive.
   */
  void change(const std::string& aLambda,
              const std::string& aDest,
              const float        aWeight) override;

  /**
   * Multiply the weight of the given destination by a factor.
   *
   * \param aLambda The name of the lambda function.
   *
   * \param aDest The destination to be added or whose weight has to be
   * changed.
   *
   * \param aFactor The weight factor.
   *
   * \throw NoDestinations if the destination is empty or not found.
   *
   * \throw InvalidWeightFactor if the weight factor is non-positive.
   */
  void multiply(const std::string& aLambda,
                const std::string& aDest,
                const float        aFactor);

  //! Remove a destination for a given lambda.
  void remove(const std::string& aLambda, const std::string& aDest) override;

  //! Remove all destinations for a given lambda.
  void remove(const std::string& aLambda) override;

  /**
   * \return the destination for the given lambda.
   *
   * \throw NoDestinations if the given lambda is not in the table.
   */
  std::string operator()(const std::string& aLambda);

  //! \return all the destinations for a given lambda.
  std::map<std::string, std::pair<float, bool>>
  destinations(const std::string& aLambda) const;

  //! \return all possible lambda served.
  std::set<std::string> lambdas() const override;

  //! \return a full representation of the table.
  std::map<std::string, std::map<std::string, std::pair<float, bool>>>
  fullTable() const override;

 private:
  const Type                                             theType;
  mutable std::mutex                                     theMutex;
  std::map<std::string, std::unique_ptr<entries::Entry>> theTable;
};

const std::string& toString(const ForwardingTable::Type aType);

ForwardingTable::Type forwardingTableTypeFromString(const std::string& aType);

} // namespace edge
} // namespace uiiit
