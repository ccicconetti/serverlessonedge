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

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace uiiit {
namespace edge {

struct InvalidNode final : public std::runtime_error {
  InvalidNode(const std::string& aName)
      : std::runtime_error("Invalid or unknown node '" + aName + "'") {
  }
};

struct InvalidTopologyFile final : public std::runtime_error {
  InvalidTopologyFile(const std::string& aName)
      : std::runtime_error("Invalid or non-existing topology file '" + aName +
                           "'") {
  }
};

/**
 * A network topology represented as the distance between any two nodes.
 *
 * Example file:
 *
 * <pre>
 * 10.0.0.1 0 3 3 2 2 2 2 3 3 3 3 3 3 3 3
 * 10.0.0.2 3 0 3 3 3 3 3 2 2 2 2 3 3 3 3
 * 10.0.0.3 3 3 0 3 3 3 3 3 3 3 3 2 2 2 2
 * 10.0.0.4 2 3 3 0 2 2 2 3 3 3 3 3 3 3 3
 * 10.0.0.5 2 3 3 2 0 2 2 3 3 3 3 3 3 3 3
 * 10.0.0.6 2 3 3 2 2 0 2 3 3 3 3 3 3 3 3
 * 10.0.0.7 2 3 3 2 2 2 0 3 3 3 3 3 3 3 3
 * 10.0.0.8 3 2 3 3 3 3 3 0 2 2 2 3 3 3 3
 * 10.0.0.9 3 2 3 3 3 3 3 2 0 2 2 3 3 3 3
 * 10.0.0.10 3 2 3 3 3 3 3 2 2 0 2 3 3 3 3
 * 10.0.0.11 3 2 3 3 3 3 3 2 2 2 0 3 3 3 3
 * 10.0.0.12 3 3 2 3 3 3 3 3 3 3 3 0 2 2 2
 * 10.0.0.13 3 3 2 3 3 3 3 3 3 3 3 2 0 2 2
 * 10.0.0.14 3 3 2 3 3 3 3 3 3 3 3 2 2 0 2
 * 10.0.0.15 3 3 2 3 3 3 3 3 3 3 3 2 2 2 0
 * </pre>
 */
class Topology final
{
 public:
  NONCOPYABLE_NONMOVABLE(Topology);

  /**
   * Create a topology from a text file.
   *
   * Empty lines are skipped. Comment lines begin with a '#' sign.
   *
   * \throw InvalidTopologyFile if there is an error in aInputFile or the file
   *         does not exist.
   */
  explicit Topology(const std::string& aInputFile);

  ~Topology();

  /**
   * \return the distance between two nodes.
   *
   * \throw InvalidNode if aSrc or aDst are not known.
   */
  double distance(const std::string& aSrc, const std::string& aDst) const;

  //! \return the number of nodes.
  size_t numNodes() const;

  //! Make the distances between nodes random.
  void randomize();

  void print(std::ostream& aStream) const;

 private:
  // key:   node name/address
  // value: node index in the distance matrix
  std::map<std::string, size_t> theNames;

  // NxN matrix of distances
  std::vector<double> theDistances;
};

} // namespace edge
} // end namespace uiiit

std::ostream& operator<<(std::ostream&                aStream,
                         const uiiit::edge::Topology& aTopology);
