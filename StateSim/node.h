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

#include "StateSim/affinity.h"
#include "StateSim/counter.h"
#include "StateSim/element.h"

#include <string>

namespace uiiit {
namespace statesim {

class Node : public Element
{
 public:
  enum class Type : int {
    Networking = 0,
    Processing = 1,
  };

  /**
   *
   * Create a networking node with given identifiers.
   *
   * \throw std::runtime_error if empty name.
   */
  explicit Node(const std::string& aName, const size_t aId);

  /**
   * Create a processing node.
   *
   * \param aType The nodetype.
   *
   * \param aName The node string identifier.
   *
   * \param aId The node numeric identifier.
   *
   * \param aSpeed The node processing speed, in FLOPS.
   *
   * \param aMemory The node main memory, in bytes.
   *
   * \param aCapacity The link capacity, in Mb/s.
   *
   * \throw std::runtime_error if empty name or non-positive speed/memory.
   */
  explicit Node(const std::string& aName,
                const size_t       aId,
                const float        aSpeed,
                const size_t       aMemory,
                const Affinity     aAffinity);

  //! \return a Node from a string.
  static Node make(const std::string& aString, Counter<int>& aCounter);
  //! \return a human-readable string.
  std::string toString() const;

  // clang-format off
  Type        type()     const noexcept { return theType;     }
  float       speed()    const noexcept { return theSpeed;    }
  size_t      memory()   const noexcept { return theMemory;   }
  Affinity    affinity() const noexcept { return theAffinity; }
  // clang-format on

 private:
  const Type theType;

  // only valid if theType == Type::Processing
  const float    theSpeed;
  const size_t   theMemory;
  const Affinity theAffinity;
};

} // namespace statesim
} // namespace uiiit
