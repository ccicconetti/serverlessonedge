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

#include "edgecontrollerflat.h"

#include <cassert>
#include <tuple>

#include <glog/logging.h>

namespace uiiit {
namespace edge {

EdgeControllerFlat::EdgeControllerFlat()
    : EdgeController() {
  LOG(INFO) << "Created a controller with flat routing";
}

void EdgeControllerFlat::privateAnnounceComputer(
    const std::string& aEdgeServerEndpoint, const ContainerList& aContainers) {
  ASSERT_IS_LOCKED(theMutex);

  // retrieve all the lambdas offered by this computer
  Entries myEntries;
  for (const auto& myContainer : aContainers.theContainers) {
    myEntries.push_back(std::make_tuple(
        myContainer.theLambda, aEdgeServerEndpoint, 1.0f, true));
  }

  // notify the lambdas to all the routers
  std::list<RouterEndpoints> myRemoveList;
  for (const auto& myRouter : theRouters.routers()) {
    if (not changeRoutes(myRouter.theForwardingTableServer, myEntries)) {
      myRemoveList.push_back(myRouter);
    }
  }

  // remove all the routers for which communication has failed
  for (const auto& myBadRouter : myRemoveList) {
    removeRouter(myBadRouter);
  }
}

void EdgeControllerFlat::privateAnnounceRouter(
    const std::string& aEdgeServerEndpoint,
    const std::string& aEdgeRouterEndpoint) {
  ASSERT_IS_LOCKED(theMutex);

  std::ignore = aEdgeServerEndpoint;

  // retrieve all the lambdas offered by all known computers
  Entries myEntries;
  for (const auto& myLambda : theComputers.lambdas()) {
    myEntries.push_back(
        std::make_tuple(myLambda.first, myLambda.second, 1.0f, true));
  }

  // announce those routes (if any) to the new router
  if (not myEntries.empty()) {
    if (not changeRoutes(aEdgeRouterEndpoint, myEntries)) {
      removeRouter(RouterEndpoints(aEdgeServerEndpoint, aEdgeRouterEndpoint));
    }
  }
}

void EdgeControllerFlat::privateRemoveComputer(
    const std::string&            aEdgeServerEndpoint,
    const std::list<std::string>& aLambdas) {
  ASSERT_IS_LOCKED(theMutex);

  // remove all lambdas of this computer from edge routers' forwarding tables
  // note we need to make a copy of theRouters because that may be changed
  // by removeRouter(), invalidating the iterators
  const auto myRoutersCopy = theRouters.routers();
  for (const auto& myRouter : myRoutersCopy) {
    if (not removeRoutes(
            myRouter.theForwardingTableServer, aEdgeServerEndpoint, aLambdas)) {
      removeRouter(myRouter);
    }
  }
}

void EdgeControllerFlat::privateRemoveRouter(
    const RouterEndpoints& aRouterEndpoints) {
}

} // namespace edge
} // namespace uiiit
