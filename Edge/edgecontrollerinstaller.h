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

#include <list>
#include <string>
#include <tuple>

namespace uiiit {
namespace edge {

/**
 * Interface for an edge controller installing forwarding entries into
 * edge routers and dispatchers.
 */
class EdgeControllerInstaller
{
 public:
  virtual ~EdgeControllerInstaller() {
  }

 protected:
  /**
   * Invoked as the update/addition of a set of lambdas, each provided by a
   * given edge computer, must be notified to a given edge router.
   *
   * \param aEndpoint The forwarding table server end-point of the router
   *        to be configured.
   *
   * \param aLambdas The table of lambdas to be configured. The tuple elements
   *        are:
   *        - 0: lambda name
   *        - 1: lambda processor end-point
   *        - 2: weight
   *        - 3: final flag
   *
   * \return true if the command is executed correctly, false otherwise.
   *         Return false is expected to cause the removal of the router
   *         from the controller.
   */
  virtual bool changeRoutes(
      const std::string& aEndpoint,
      const std::list<std::tuple<std::string, std::string, float, bool>>&
          aLambdas) = 0;

  /**
   * Invoked as the removal of the given set of lambdas served by an edge
   * computer must be notified to a given edge router.
   *
   * \param aEdgeRouterEndpoint The end-point of the forwarding table server.
   *
   * \param aEdgeComputerEndpoint The end-point of the lambda processor.
   *
   * \param aLambdas The list of lambdas to be removed from the router's table.
   *
   * \return true if the command is executed correctly, false otherwise.
   *         Return false is expected to cause the removal of the router
   *         from the controller.
   */
  virtual bool removeRoutes(const std::string&            aEdgeRouterEndpoint,
                            const std::string&            aEdgeComputerEndpoint,
                            const std::list<std::string>& aLambdas) = 0;

  /**
   * Invoked to flush all the forwarding table entries of a router.
   *
   * \param aEdgeRouterEndpoint The forwarding table server end-point.
   *
   * \return true if the command is executed correctly, false otherwise.
   *         Return false is expected to cause the removal of the router
   *         from the controller.
   */
  virtual bool flushRoutes(const std::string& aEdgeRouterEndpoint) = 0;
};

} // end namespace edge
} // end namespace uiiit
