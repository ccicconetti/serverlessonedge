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

#include "Edge/edgecontrollerinstaller.h"
#include "Edge/edgecontrollermessages.h"

#include <list>
#include <string>
#include <tuple>

namespace uiiit {
namespace edge {

struct TrivialEdgeControllerInstaller : virtual public EdgeControllerInstaller {
  TrivialEdgeControllerInstaller();

  bool changeRoutes(
      const std::string& aEndpoint,
      const std::list<std::tuple<std::string, std::string, float, bool>>&
          aLambdas) override;

  bool removeRoutes(const std::string&            aEdgeRouterEndpoint,
                    const std::string&            aEdgeComputerEndpoint,
                    const std::list<std::string>& aLambdas) override;

  bool flushRoutes(const std::string& aEdgeRouterEndpoint) override;

  std::string logGetAndClear();

  std::string theLog;
  std::string theDisconnected;
};

////////////////////////////////////////////////////////////////////////////////
// free functions

ContainerList makeContainers(size_t       aNumLambdas,
                             const size_t aStartingFrom = 0);

} // namespace edge
} // namespace uiiit
