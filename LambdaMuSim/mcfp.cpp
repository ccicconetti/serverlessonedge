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

#include "LambdaMuSim/mcfp.h"

#include "Support/tostring.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_concepts.hpp>
#include <boost/graph/successive_shortest_path_nonnegative_weights.hpp>

#include <numeric>
#include <stdexcept>
#include <string>

#include <glog/logging.h>

namespace uiiit {
namespace lambdamusim {

//
// minor modifications to the code in:
//
// https://www.boost.org/doc/libs/1_78_0/libs/graph/example/successive_shortest_path_nonnegative_weights_example.cpp
//
// authored by Piotr Wygocki, 2013, and distributed under the
// Boost Software License, Version 1.0
//

template <class Graph,
          class Weight,
          class Capacity,
          class Reversed,
          class ResidualCapacity>
class EdgeAdder
{
  using Traits =
      boost::adjacency_list_traits<boost::vecS, boost::vecS, boost::directedS>;
  using vertex_descriptor = Traits::vertex_descriptor;

 public:
  EdgeAdder(Graph& g, Weight& w, Capacity& c, Reversed& rev, ResidualCapacity&)
      : m_g(g)
      , m_w(w)
      , m_cap(c)
      , m_rev(rev) {
  }
  void addEdge(const vertex_descriptor v,
               const vertex_descriptor w,
               const double            weight,
               const long              capacity) {
    Traits::edge_descriptor e, f;
    e        = add(v, w, weight, capacity);
    f        = add(w, v, -weight, 0);
    m_rev[e] = f;
    m_rev[f] = e;
  }

 private:
  Traits::edge_descriptor add(const vertex_descriptor v,
                              const vertex_descriptor w,
                              const double            weight,
                              const long              capacity) {
    bool                    b;
    Traits::edge_descriptor e;
    boost::tie(e, b) = add_edge(vertex(v, m_g), vertex(w, m_g), m_g);
    if (not b) {
      throw std::runtime_error("Edge between " + std::to_string(v) + " and " +
                               std::to_string(w) + " already exists");
    }
    m_cap[e] = capacity;
    m_w[e]   = weight;
    return e;
  }

  Graph&    m_g;
  Weight&   m_w;
  Capacity& m_cap;
  Reversed& m_rev;
};

double Mcfp::solve(const Costs&      aCosts,
                   const Requests&   aRequests,
                   const Capacities& aCapacities,
                   Weights&          aWeights) {
  // consistency checks
  if (aCosts.empty()) {
    throw std::runtime_error("Invalid empty cost matrix");
  }

  using Traits =
      boost::adjacency_list_traits<boost::vecS, boost::vecS, boost::directedS>;
  using Graph = boost::adjacency_list<
      boost::vecS,
      boost::vecS,
      boost::directedS,
      boost::no_property,
      boost::property<
          boost::edge_capacity_t,
          long,
          boost::property<
              boost::edge_residual_capacity_t,
              long,
              boost::property<boost::edge_reverse_t,
                              Traits::edge_descriptor,
                              boost::property<boost::edge_weight_t, double>>>>>;
  using Capacity = boost::property_map<Graph, boost::edge_capacity_t>::type;
  using ResidualCapacity =
      boost::property_map<Graph, boost::edge_residual_capacity_t>::type;
  using Weight    = boost::property_map<Graph, boost::edge_weight_t>::type;
  using Reversed  = boost::property_map<Graph, boost::edge_reverse_t>::type;
  using size_type = boost::graph_traits<Graph>::vertices_size_type;
  using vertex_descriptor = Traits::vertex_descriptor;

  Graph            g;
  Capacity         capacity = boost::get(boost::edge_capacity, g);
  Reversed         rev      = boost::get(boost::edge_reverse, g);
  ResidualCapacity residual_capacity =
      boost::get(boost::edge_residual_capacity, g);
  Weight weight = boost::get(boost::edge_weight, g);

  // count the number of tasks and workers and run consistency checks
  const auto T = aCosts.size();    // number of tasks
  const auto W = aCosts[0].size(); // number of workers
  for (const auto& myCosts : aCosts) {
    if (myCosts.size() != W) {
      throw std::runtime_error("Invalid cost matrix: expected " +
                               std::to_string(W) + " workers, found " +
                               std::to_string(myCosts.size()));
    }
  }
  if (aRequests.size() != T) {
    throw std::runtime_error("Invalid requests size: expected " +
                             std::to_string(T) + ", found " +
                             std::to_string(aRequests.size()));
  }
  if (aCapacities.size() != W) {
    throw std::runtime_error("Invalid capacity size: expected " +
                             std::to_string(W) + ", found " +
                             std::to_string(aCapacities.size()));
  }

  // create the graph

  // source:      0
  // tasks:       1     .. T
  // workers:     T+1   .. T+W
  // workers':    T+W+1 .. T+2W
  // destination: last == N-1 == T+2W+1
  const size_type N = T + 2 * W + 2;
  for (size_type i = 0; i < N; ++i) {
    add_vertex(g);
  }
  vertex_descriptor s = 0;
  vertex_descriptor d = N - 1;

  // the max capacity is the sum of the capacities on all workers
  // which makes sure that on virtual edges this never becomes a constraint
  const long myMaxCap =
      std::accumulate(aCapacities.begin(), aCapacities.end(), 0);

  EdgeAdder<Graph, Weight, Capacity, Reversed, ResidualCapacity> ea(
      g, weight, capacity, rev, residual_capacity);

  // add edges between the source and the tasks
  for (size_type t = 1; t <= T; t++) {
    ea.addEdge(s, t, 0, aRequests[t - 1]);
  }

  // add edges between tasks and workers
  for (size_type t = 1; t <= T; t++) {
    for (size_type w = (T + 1); w <= (T + W); w++) {
      ea.addEdge(t, w, aCosts[t - 1][w - (T + 1)], myMaxCap);
    }
  }

  // add edges between workers and workers'
  for (size_type w = (T + 1); w <= (T + W); w++) {
    ea.addEdge(w, w + W, 0, aCapacities[w - (T + 1)]);
  }

  // add edges between the workers' and the destination
  for (size_type w = (T + W + 1); w <= (T + 2 * W); w++) {
    ea.addEdge(w, d, 0, myMaxCap);
  }

  // solve the minimum cost flow problem
  boost::successive_shortest_path_nonnegative_weights(g, s, d);

  // compute the cost and return it, along with the weights
  aWeights   = Weights(T, std::vector<double>(W));
  double ret = 0;
  for (size_type t = 1; t <= T; t++) {
    for (size_type w = (T + 1); w <= (T + W); w++) {
      Traits::edge_descriptor e;
      [[maybe_unused]] bool   found;
      std::tie(e, found) = boost::edge(t, w, g);
      assert(found);
      const auto myCost = aCosts[t - 1][w - (T + 1)];
      assert(weight(e) == myCost);
      const auto myCapacity = capacity(e);
      if (myCapacity > 0) {
        const auto myResidual = residual_capacity(e);

        assert(myCapacity >= myResidual);
        assert((t - 1) < aWeights.size());
        assert((w - T - 1) < aWeights[t - 1].size());

        const auto myWeight          = myCapacity - myResidual;
        aWeights[t - 1][w - (T + 1)] = myWeight;
        ret += myWeight * myCost;
      }
    }
  }
  if (VLOG_IS_ON(2)) {
    std::stringstream myStream;
    for (const auto& elem : aWeights) {
      myStream << ::toString(elem, "\t", [](const auto& aValue) {
        return std::to_string(aValue);
      }) << '\n';
    }
    VLOG(2) << "cost " << ret << ", weights\n" << myStream.str();
  }

  return ret;
}

} // namespace lambdamusim
} // namespace uiiit
