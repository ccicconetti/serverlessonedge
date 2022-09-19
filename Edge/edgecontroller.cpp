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

#include "edgecontroller.h"

#include "Support/tostring.h"

#include <glog/logging.h>

#include <cassert>

namespace uiiit {
namespace edge {

EdgeController::Computers::AddStatus
EdgeController::Computers::add(const std::string&   aEdgeServerEndpoint,
                               const ContainerList& aContainers) {
  const auto it = theComputers.insert({aEdgeServerEndpoint, aContainers});
  if (not it.second) {
    if (it.first->second == aContainers) {
      return AddStatus::AlreadyPresent;
    }
    // do not update the data structure
    return AddStatus::ContainersChanged;
  }
  return AddStatus::NotPresent;
}

std::list<std::string>
EdgeController::Computers::remove(const std::string& aEdgeServerEndpoint) {
  std::list<std::string> myRet;
  const auto             it = theComputers.find(aEdgeServerEndpoint);
  if (it != theComputers.end()) {
    // save all the lambdas for return
    for (const auto& myContainer : it->second.theContainers) {
      myRet.push_back(myContainer.theLambda);
    }

    // remove the computer
    theComputers.erase(it);
  }
  return myRet;
}

std::list<std::pair<std::string, std::string>>
EdgeController::Computers::lambdas() const {
  std::list<std::pair<std::string, std::string>> myRet;
  for (const auto& myComputer : theComputers) {
    for (const auto& myContainer : myComputer.second.theContainers) {
      myRet.push_back({myContainer.theLambda, myComputer.first});
    }
  }
  return myRet;
}

const std::map<std::string, ContainerList>&
EdgeController::Computers::computers() const {
  return theComputers;
}

void EdgeController::Computers::print(std::ostream& aStream) const {
  for (const auto& myComputer : theComputers) {
    aStream << "computer " << myComputer.first << '\n' << myComputer.second;
  }
}

void EdgeController::Routers::add(const std::string& aEdgeServerEndpoint,
                                  const std::string& aEdgeRouterEndpoint) {
  const auto res = theRouters.emplace(aEdgeServerEndpoint, aEdgeRouterEndpoint);
  if (not res.second) {
    res.first->second = aEdgeRouterEndpoint;
  }
  VLOG_IF(1, not res.second) << "duplicate edge router " << aEdgeServerEndpoint
                             << "|" << aEdgeRouterEndpoint << " added";
}

bool EdgeController::Routers::remove(const std::string& aEdgeServerEndpoint) {
  LOG(INFO) << "removing edge router " << aEdgeServerEndpoint
            << " from the known routers";
  return theRouters.erase(aEdgeServerEndpoint) > 0;
}

std::string EdgeController::Routers::forwardingServerEndpoint(
    const std::string& aEdgeServerEndpoint) const {
  const auto it = theRouters.find(aEdgeServerEndpoint);
  if (it == theRouters.end()) {
    return std::string();
  }
  return it->second;
}

const std::map<std::string, std::string>&
EdgeController::Routers::routers() const noexcept {
  return theRouters;
}

void EdgeController::Routers::print(std::ostream& aStream) const {
  aStream << toString(theRouters, ",", [](const auto& pair) {
    return pair.first + "|" + pair.second;
  });
}

EdgeController::EdgeController()
    : theMutex()
    , theComputers()
    , theRouters()
    , theLambdas() {
}

void EdgeController::announceComputer(const std::string&   aEdgeServerEndpoint,
                                      const ContainerList& aContainers) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  const auto myStatus = theComputers.add(aEdgeServerEndpoint, aContainers);
  LOG(INFO) << "computer announce from " << aEdgeServerEndpoint << ":\n"
            << aContainers;

  // if the computer is already present with the same containers we do nothing
  if (myStatus == Computers::AddStatus::AlreadyPresent) {
    return;
  }

  //
  // if the computer was already present but with different containers, we
  // remove the old computer before proceeding
  //
  // note: if there are lambdas that were already present in the computer before
  // and they are confirmed by this announce, some spurious calls to
  // delLambda/addLambda will be done to derived classes
  //
  if (myStatus == Computers::AddStatus::ContainersChanged) {
    privateRemoveComputer(aEdgeServerEndpoint);
    const auto myNewStatus = theComputers.add(aEdgeServerEndpoint, aContainers);
    assert(myNewStatus == Computers::AddStatus::NotPresent);
    std::ignore = myNewStatus;
  }

  // retrieve all the lambdas offered by this computer
  for (const auto& myContainer : aContainers.theContainers) {
    auto& myComputerSet = theLambdas[myContainer.theLambda];
    if (myComputerSet.empty()) {
      // notify derived classes that a new lambda has just appeared
      addLambda(myContainer.theLambda);
    }
    // mark that the current lambda is served (also) by this computer
    myComputerSet.insert(aEdgeServerEndpoint);
  }

  // notify derived classes that a new computer has been added
  std::list<std::string> myLambdas;
  for (const auto& myContainer : aContainers.theContainers) {
    myLambdas.push_back(myContainer.theLambda);
  }
  addComputer(aEdgeServerEndpoint, myLambdas);

  privateAnnounceComputer(aEdgeServerEndpoint, aContainers);
}

void EdgeController::announceRouter(const std::string& aEdgeServerEndpoint,
                                    const std::string& aEdgeRouterEndpoint) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  theRouters.add(aEdgeServerEndpoint, aEdgeRouterEndpoint);
  LOG(INFO) << "router announce from " << aEdgeServerEndpoint
            << " with forwarding table server " << aEdgeRouterEndpoint;

  privateAnnounceRouter(aEdgeServerEndpoint, aEdgeRouterEndpoint);
}

void EdgeController::removeComputer(const std::string& aEdgeServerEndpoint) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  privateRemoveComputer(aEdgeServerEndpoint);
}

void EdgeController::privateRemoveComputer(
    const std::string& aEdgeServerEndpoint) {
  ASSERT_IS_LOCKED(theMutex);

  // remove the computer from the list and retrieve its lambdas
  const auto myLambdas = theComputers.remove(aEdgeServerEndpoint);

  LOG(INFO) << "computer removed " << aEdgeServerEndpoint
            << ", lambdas served: " << toString(myLambdas, ", ");

  // remove all associations of this edge computer from its lambdas
  for (const auto& myLambda : myLambdas) {
    auto it = theLambdas.find(myLambda);
    if (it != theLambdas.end()) {
      const auto ret = it->second.erase(aEdgeServerEndpoint);
      LOG_IF(WARNING, ret != 1)
          << "trying to remove lambda " << myLambda << " from computer "
          << aEdgeServerEndpoint
          << " but such association does not exist: ignored";

      // remove the current lambda if this was the last edge computer serving it
      // and notify derived classes
      if (it->second.empty()) {
        theLambdas.erase(it);
        delLambda(myLambda);
      }

    } else {
      LOG(WARNING) << "trying to remove edge computer " << aEdgeServerEndpoint
                   << " whose lambda " << myLambda << " is not known: ignored";
    }
  }

  // notify derived classes that this computer has been removed
  delComputer(aEdgeServerEndpoint, myLambdas);

  privateRemoveComputer(aEdgeServerEndpoint, myLambdas);
}

void EdgeController::removeRouter(const std::string& aRouterEndpoint) {
  ASSERT_IS_LOCKED(theMutex);

  theRouters.remove(aRouterEndpoint);

  privateRemoveRouter(aRouterEndpoint);
}

void EdgeController::print(std::ostream& aStream) const {
  const std::lock_guard<std::mutex> myLock(theMutex);
  theComputers.print(aStream);
  theRouters.print(aStream);
}

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                      aStream,
                         const uiiit::edge::EdgeController& aController) {
  aController.print(aStream);
  return aStream;
}
