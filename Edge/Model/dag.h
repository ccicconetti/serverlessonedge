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

#include "Edge/Model/states.h"
#include "Support/macros.h"

#include <set>
#include <string>
#include <vector>

namespace uiiit {
namespace edge {
namespace model {

/**
 * @brief A Directed Acyclic Graph (DAG) of stateful function invocations.
 *
 * The DAG always has a single entry function and a single terminating one.
 *
 * Objects of this class are immutable.
 */
class Dag final
{
 public:
  // for each entry: all the indices of the functions that follow this one
  // the 0-th entry is always the starting point of the invocation
  // the number of entries is the number of functions in the DAG - 1
  // (as the last function does not have any successor)
  using Successors    = std::vector<std::vector<size_t>>;
  using Predecessors  = Successors;
  using FunctionNames = std::vector<std::string>;
  using Dependencies  = States::Dependencies;

  /**
   * @brief Construct a new Dag object.
   *
   * @param aSuccessors The DAG of function, with indices.
   *
   * @param aFunctionNames The names of the functions by their indices.
   *
   * @param theDependencies The dependencies of functions from states.
   *
   * @throw std::runtime_error if some states depend on non-existing
   * functions or the size of the successors and function names are not
   * consistent.
   */
  explicit Dag(const Successors&    aSuccessors,
               const FunctionNames& aFunctionNames,
               const Dependencies&  aDependencies);

  ~Dag();

  //! @return true if the dags are the same.
  bool operator==(const Dag& aOther) const;

  //! @return the chain name obtained as a mangle of the function names.
  std::string name() const;

  //! @return the set of unique function names in the chain.
  std::set<std::string> uniqueFunctions() const;

  //! @return the DAG of functions, as successors, referred by their indices.
  const Successors& successors() const;

  //! @return the names of the successors of a given function, by its index.
  std::set<std::string> successorNames(const size_t aIndex) const;

  //! @return the names of the predecessors of a given function, by its index.
  std::set<std::string> predecessorNames(const size_t aIndex) const;

  //! @return the function names, by their indices.
  const FunctionNames& functionNames() const;

  //! @return the state dependencies.
  const States& states() const;

  //! @return the name of the first function of the DAG.
  std::string entryFunctionName() const;

  //! @return the name of the function or an empty string.
  std::string toName(const size_t aIndex) const;

  //! @return the functions if the passed ones are completed.
  std::set<size_t> callable(const std::set<size_t>& aCompleted) const;

  //! @return a human-readable single-line string.
  std::string toString() const;

  /**
   * @brief Create a DAG from a JSON-encoded string.
   *
   * @param aJson The JSON-encoded string.
   *
   * @return the newly created Dag.
   *
   * @throw std::runtime_error if the JSON has invalid or inconsistent text.
   */
  static Dag fromJson(const std::string& aJson);

  //! Convert to a human-readable JSON-encoded string.
  std::string toJson() const;

 private:
  static Predecessors makePredecessors(const Successors& aSuccessors);

  //! @return all ancestors of the function.
  std::set<size_t> ancestors(const size_t aIndex) const;

  //! @return a vector with the name of the functions.
  template <class CONTAINER>
  CONTAINER toNames(const std::vector<size_t>& aIndices) const {
    CONTAINER ret;
    for (const auto& myIndex : aIndices) {
      ret.insert(ret.end(), toName(myIndex));
    }
    return ret;
  }

 private:
  const Successors    theSuccessors;
  const Predecessors  thePredecessors;
  const FunctionNames theFunctionNames;
  const States        theStates;
};

// free functions

//! @return an example chain.
Dag exampleDag();

} // namespace model
} // namespace edge
} // namespace uiiit
