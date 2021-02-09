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

#include "StateSim/element.h"
#include "StateSim/link.h"
#include "StateSim/node.h"
#include "Support/macros.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>

#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace uiiit {
namespace statesim {

/**
 * Model a network of nodes and links.
 *
 * Routing information is lazy-initialized and cached for performance reasons.
 *
 * The class is thread-safe.
 */
class Network
{
  NONCOPYABLE_NONMOVABLE(Network);

  using Graph =
      boost::adjacency_list<boost::listS,
                            boost::vecS,
                            boost::directedS,
                            boost::no_property,
                            boost::property<boost::edge_weight_t, float>,
                            boost::no_property,
                            boost::listS>;
  using VertexDescriptor = boost::graph_traits<Graph>::vertex_descriptor;
  using Edge             = std::pair<int, int>;
  using PredVec          = std::vector<std::pair<float, VertexDescriptor>>;
  using Cache            = std::vector<std::pair<bool, PredVec>>;

 public:
  /**
   * Create a network from a set of files.
   *
   * All non-server nodes are identified as clients.
   *
   * \param aNodesPath The file containing the nodes in the following format:
   *        0;rpi3_0;1023012864;4000;arm32;sbc;rpi3b+
   *
   * \param aLinksPath The file containing the links in the following format:
   *        0;1000;link_rpi3_0;node
   *
   * \param aEdgesPath The file containing the edges in the following format:
   *        switch_lan_0 link_rpi3_0 link_rpi3_1 link_0
   */
  explicit Network(const std::string& aNodesPath,
                   const std::string& aLinksPath,
                   const std::string& aEdgesPath);

  /**
   * Create a network from nodes, links, and edges.
   *
   * \param aNodes The set of nodes, both processing and non-processing
   *
   * \param aLinks The set of links
   *
   * \param aEdges The adjecency list of edges, i.e., for every <key,value>
   *        key is the name of an element (node or link) and value is the set of
   *        elements that have a directed edge towards it
   *
   * \param aClients The set of client names
   *
   * \throw std::runtime_error if there is any inconsistency such as aClients
   *        or aEdges containing a name that is not a node or aNodes/aLinks
   *        having duplicated identifiers
   */
  explicit Network(const std::set<Node>&                               aNodes,
                   const std::set<Link>&                               aLinks,
                   const std::map<std::string, std::set<std::string>>& aEdges,
                   const std::set<std::string>& aClients);

  // clang-format off
  const std::map<std::string, Node>& nodes()      const noexcept { return theNodes; }
  const std::map<std::string, Link>& links()      const noexcept { return theLinks; }
  const std::vector<Node*>&          clients()    const noexcept { return theClients; }
  const std::vector<Node*>&          processing() const noexcept { return theProcessing; }
  // clang-format on

  //! \return the distance and next hop identifier from aSrc to aDst
  std::pair<float, std::string> nextHop(const std::string& aSrc,
                                        const std::string& aDst);

  //! \return the transmission time from aSrc to aDst of a given amount of data
  double txTime(const Node& aSrc, const Node& aDst, const size_t aBytes);

  //! \return the number of hops between two nodes
  size_t hops(const Node& aSrc, const Node& aDst);

 private:
  //! Convert name to numeric identifier.
  size_t id(const std::string& aName) const;
  //! \return the capacity of the given element.
  float capacity(const std::string& aName) const;

  //! \return the cache entry for the given node, create it if does not exist
  const Cache::value_type& cacheEntry(const size_t aDstId);

  //! \return the cache entry for the given node, throw if it does not exist
  const Cache::value_type& cacheEntry(const size_t aDstId) const;

  void initElementsGraph(const std::vector<Edge>&  aEdges,
                         const std::vector<float>& aWeights);

 private:
  mutable std::mutex          theMutex;
  std::map<std::string, Node> theNodes;
  std::map<std::string, Link> theLinks;
  std::vector<Element*>       theElements;
  std::vector<Node*>          theClients;
  std::vector<Node*>          theProcessing;
  Graph                       theGraph;

  //
  // first index: source node id
  // second index: <valid flag, destination node id>
  // content: <distance from source to destination, next hop>
  //
  // lazy-initialized as needed
  Cache theCache;
}; // namespace statesim

std::vector<Node> loadNodes(const std::string& aPath, Counter<int>& aCounter);
std::vector<Link> loadLinks(const std::string& aPath, Counter<int>& aCounter);

} // namespace statesim
} // namespace uiiit
