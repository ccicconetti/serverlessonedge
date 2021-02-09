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

#include "Edge/edgecontroller.h"
#include "Edge/forwardingtableclient.h"

#include <glog/logging.h>

#include <cassert>
#include <string>

namespace uiiit {
namespace edge {

/**
 * Edge controller that announces/removes routes via gRPC.
 *
 * Template based on the actual type of controller used.
 */
template <class CONTROLLER>
class EdgeControllerRpc final : public CONTROLLER
{
 public:
  explicit EdgeControllerRpc()
      : CONTROLLER() {
  }

 private:
  bool changeRoutes(
      const std::string& aEndpoint,
      const std::list<std::tuple<std::string, std::string, float, bool>>&
          aLambdas) override;

  bool removeRoutes(const std::string&            aEdgeRouterEndpoint,
                    const std::string&            aEdgeComputerEndpoint,
                    const std::list<std::string>& aLambdas) override;

  bool flushRoutes(const std::string& aEdgeRouterEndpoint) override;

  bool forwardingTableCommand(const std::string&               aEndpoint,
                              const std::function<void(void)>& aCommand);
};

////////////////////////////////////////////////////////////////////////////////
// Implementation

template <class CONTROLLER>
bool EdgeControllerRpc<CONTROLLER>::changeRoutes(
    const std::string& aEndpoint,
    const std::list<std::tuple<std::string, std::string, float, bool>>&
        aLambdas) {
  ForwardingTableClient myClient(aEndpoint);
  return forwardingTableCommand(aEndpoint, [&]() {
    for (const auto& myLambda : aLambdas) {
      myClient.change(std::get<0>(myLambda),
                      std::get<1>(myLambda),
                      std::get<2>(myLambda),
                      std::get<3>(myLambda));
    }
  });
}

template <class CONTROLLER>
bool EdgeControllerRpc<CONTROLLER>::removeRoutes(
    const std::string&            aEdgeRouterEndpoint,
    const std::string&            aEdgeComputerEndpoint,
    const std::list<std::string>& aLambdas) {
  ForwardingTableClient myClient(aEdgeRouterEndpoint);
  return forwardingTableCommand(aEdgeRouterEndpoint, [&]() {
    for (const auto& myLambda : aLambdas) {
      myClient.remove(myLambda, aEdgeComputerEndpoint);
    }
  });
}

template <class CONTROLLER>
bool EdgeControllerRpc<CONTROLLER>::flushRoutes(
    const std::string& aEdgeRouterEndpoint) {
  ForwardingTableClient myClient(aEdgeRouterEndpoint);
  return forwardingTableCommand(aEdgeRouterEndpoint,
                                [&]() { myClient.flush(); });
}

template <class CONTROLLER>
bool EdgeControllerRpc<CONTROLLER>::forwardingTableCommand(
    const std::string& aEndpoint, const std::function<void(void)>& aCommand) {
  assert(aCommand);
  try {
    aCommand();
    return true;
  } catch (const std::runtime_error& aErr) {
    LOG(ERROR) << "Could not issue a command to edge router at " << aEndpoint
               << ": " << aErr.what();
  } catch (...) {
    LOG(ERROR) << "Unknown error when issuing a command to edge router at "
               << aEndpoint;
  }
  return false;
}

} // end namespace edge
} // end namespace uiiit
