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

#include <iostream>
#include <map>
#include <set>
#include <string>

namespace uiiit {
namespace edge {

/**
 * Interface for forwarding table objects, manipulated by an instance of
 * ForwardingTableServer.
 */
class ForwardingTableInterface
{
 public:
  virtual ~ForwardingTableInterface() {
  }

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
   */
  virtual void change(const std::string& aLambda,
                      const std::string& aDest,
                      const float        aWeight,
                      const bool         aFinal) = 0;

  /**
   * Change the weight of an existing lambda.
   *
   * \param aLambda The name of the lambda function.
   *
   * \param aDest The destination to be added or whose weight has to be
   * changed.
   *
   * \param aWeight The destination weight.
   */
  virtual void change(const std::string& aLambda,
                      const std::string& aDest,
                      const float        aWeight) = 0;

  //! Remove a destination for a given lambda.
  virtual void remove(const std::string& aLambda, const std::string& aDest) = 0;

  //! Remove all destinations for a given lambda.
  virtual void remove(const std::string& aLambda) = 0;

  //! \return all possible lambda served.
  virtual std::set<std::string> lambdas() const = 0;

  //! \return a full representation of the table.
  virtual std::map<std::string, std::map<std::string, std::pair<float, bool>>>
  fullTable() const = 0;
};

////////////////////////////////////////////////////////////////////////////////
// free functions

//! Add fake entries for a set of lambdas.
void fakeFill(ForwardingTableInterface& aTable,
    const size_t aNumLambdas,
    const size_t aNumDestinations);

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                                aStream,
                         const uiiit::edge::ForwardingTableInterface& aTable);
