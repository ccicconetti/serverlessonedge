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

#include "StateSim/counter.h"
#include "StateSim/element.h"

#include <string>

namespace uiiit {
namespace statesim {

class Link : public Element
{
 public:
  enum class Type : int {
    Node     = 0,
    Shared   = 1,
    Downlink = 2,
    Uplink   = 3,
  };

  /**
   *  Create a link.
   *
   * \param aType The link type.
   *
   * \param aName The link string identifier.
   *
   * \param aId The link numeric identifier.
   *
   * \param aCapacity The link capacity, in Mb/s.
   *
   * \throw std::runtime_error if the name is empty of the capacity is null.
   */
  explicit Link(const Type         aType,
                const std::string& aName,
                const size_t       aId,
                const float        aCapacity);

  //! \return a Node from a string.
  static Link make(const std::string& aString, Counter<int>& aCounter);
  //! \return a human-readable string.
  std::string toString() const;

  // clang-format off
  Type        type()     const noexcept { return theType;     }
  float       capacity() const noexcept { return theCapacity; }
  // clang-format on

  double txTime(const size_t aBytes) const override;

 private:
  const Type  theType;
  const float theCapacity;
};

} // namespace statesim
} // namespace uiiit
