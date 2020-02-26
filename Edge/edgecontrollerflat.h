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

#include "edgecontroller.h"
#include "edgecontrollerinstaller.h"

namespace uiiit {
namespace edge {

/**
 * Edge controller announcing all computers to all routers:
 *
 * when a new computer announces itself a new route is added to all the routers;
 * when a new router announces itself, all the existing routers are installed.
 *
 * The actual announce/removal of routes is left to further derived classes.
 */
class EdgeControllerFlat : public EdgeController, virtual public EdgeControllerInstaller
{
 public:
  explicit EdgeControllerFlat();

 private:
  void addComputer(const std::string&            aEndpoint,
                   const std::list<std::string>& aLambdas) override {
  }

  void delComputer(const std::string&            aEndpoint,
                   const std::list<std::string>& aLambdas) override {
  }

  void addLambda(const std::string& aLambda) override {
  }

  void delLambda(const std::string& aLambda) override {
  }

  void privateAnnounceComputer(const std::string&   aEdgeServerEndpoint,
                                       const ContainerList& aContainers) override;

  void
  privateAnnounceRouter(const std::string& aEdgeServerEndpoint,
                        const std::string& aEdgeRouterEndpoint) override;

  void privateRemoveComputer(const std::string&            aEdgeServerEndpoint,
                             const std::list<std::string>& aLambdas) override;

  void privateRemoveRouter(const RouterEndpoints& aRouterEndpoints) override;
};

} // namespace edge
} // namespace uiiit
