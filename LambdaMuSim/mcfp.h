/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2022 C. Cicconetti <https://ccicconetti.github.io/>

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

#include <vector>

namespace uiiit {
namespace lambdamusim {

/**
 * @brief Helper class to solve the minimum cost flow problem using BGL.
 *
 * Pointers to the BGL documentation (version 1.78):
 *
 * https://www.boost.org/doc/libs/1_78_0/libs/graph/doc/graph_theory_review.html#sec:network-flow-algorithms
 * https://www.boost.org/doc/libs/1_78_0/libs/graph/doc/successive_shortest_path_nonnegative_weights.html
 */
class Mcfp
{
 public:
  using Costs      = std::vector<std::vector<double>>;
  using Requests   = std::vector<long>;
  using Capacities = std::vector<long>;

  /**
   * @brief Compute the minimum cost flow in the given problem: there are a
   * number of tasks that need to be allocated to a set of workers. Each
   * task-worker has a given cost, while the
   *
   * @param aCosts The task-worker costs.
   * @param aRequests The amount of work each task requires.
   * @param aCapacities The workers' capacities.
   * @return the minimum cost found.
   *
   * @throw std::runtime_error if the input passed is inconsistent.
   */
  static double solve(const Costs&      aCosts,
                      const Requests&   aRequests,
                      const Capacities& aCapacities);
};

} // namespace lambdamusim
} // namespace uiiit
