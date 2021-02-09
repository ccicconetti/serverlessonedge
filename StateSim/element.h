/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2021 C. Cicconetti <https://ccicconetti.github.io/>

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

#include <string>

namespace uiiit {
namespace statesim {

/**
 * A generic element in the architecture identified by a string and numeric ID.
 */
class Element
{
 public:
  enum class Device : int {
    Link = 0,
    Node = 1,
  };

  /**
   *  Create an Element.
   *
   * \param aName The string identifier
   *
   * \param aId The numeric identifier
   *
   * \param aCategory The device
   *
   * \throw std::runtime_error if the name is empty
   */
  explicit Element(const std::string& aName,
                   const size_t       aId,
                   const Device       aDevice);

  virtual ~Element();

  // clang-format off
  std::string name()     const noexcept { return theName;     }
  size_t      id()       const noexcept { return theId;       }
  Device      device()   const noexcept { return theDevice;   }
  // clang-format on

  //! \return a human-readable string.
  virtual std::string toString() const = 0;

  //! \return the transmission time to traverse this element, in s
  virtual double txTime(const size_t aBytes) const {
    return 0;
  }

  bool operator<(const Element& aOther) const noexcept {
    return theId < aOther.theId;
  }

 protected:
  const std::string theName;
  const size_t      theId;
  const Device      theDevice;
};

} // namespace statesim
} // namespace uiiit
