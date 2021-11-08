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

#include "Support/macros.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace uiiit {
namespace edge {
namespace model {

/**
 * @brief Dependencies of function on the application's states.
 *
 * Objects of this class are immutable.
 */
class States final
{
 public:
  // key: state name
  // value: vector of the names of function that depend on the state
  using Dependencies = std::map<std::string, std::vector<std::string>>;

  /**
   * @brief Construct a new States object.
   *
   * @param theDependencies The dependencies of functions from states.
   *
   * @throw std::runtime_error if some states depend from non-existing
   * functions.
   */
  explicit States(const Dependencies& aDependencies);

  ~States();

  //! @return true if the chains are the same.
  bool operator==(const States& aOther) const;

  //! @return the state dependencies.
  const Dependencies& dependencies() const;

  //! @return the dependencies of a given function.
  Dependencies dependencies(const std::string& aFunction) const;

  /**
   * @brief Return the states.
   *
   * @param aIncludeFreeStates if true then also include the states with no
   * dependencies.
   *
   * @return the states of the chain.
   */
  std::set<std::string> allStates(const bool aIncludeFreeStates) const;

  //! @return the states that this function requires.
  std::set<std::string> states(const std::string& aFunction) const;

  //! @return a human-readable single-line string.
  std::string toString() const;

  /**
   * @brief Create a chain from a JSON-encoded string.
   *
   * @param aJson The JSON-encoded string.
   *
   * @return the newly created Chain.
   *
   * @throw std::runtime_error if the JSON has invalid or inconsistent text.
   */
  static States fromJson(const std::string& aJson);

  //! Convert to a human-readable JSON-encoded string.
  std::string toJson() const;

 private:
  const Dependencies theDependencies;
};

} // namespace model
} // namespace edge
} // namespace uiiit
