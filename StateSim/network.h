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

#include "StateSim/link.h"
#include "StateSim/node.h"
#include "Support/macros.h"

#include <map>
#include <string>
#include <vector>

namespace uiiit {
namespace statesim {

class Network
{
  NONCOPYABLE_NONMOVABLE(Network);

 public:
  /**
   * Create a network from a set of files.
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
  Network(const std::string& aNodesPath,
          const std::string& aLinksPath,
          const std::string& aEdgesPath);

  // clang-format off
  const std::map<std::string, Node>& nodes() const noexcept { return theNodes; }
  const std::map<std::string, Link>& links() const noexcept { return theLinks; }
  // clang-format on

 private:
  std::map<std::string, Node> theNodes;
  std::map<std::string, Link> theLinks;
};

std::vector<Node> loadNodes(const std::string& aPath);
std::vector<Link> loadLinks(const std::string& aPath);

} // namespace statesim
} // namespace uiiit
