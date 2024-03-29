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

#include "Edge/stateserver.h"

#include <glog/logging.h>
#include <grpc++/grpc++.h>

#include <cassert>

namespace uiiit {
namespace edge {

StateServer::StateServerImpl::StateServerImpl()
    : theMutex()
    , theStateRepo() {
  // noop
}

grpc::Status StateServer::StateServerImpl::Get(
    [[maybe_unused]] grpc::ServerContext* aContext,
    const rpc::State*                     aState,
    rpc::StateResponse*                   aResponse) {
  assert(aState);
  assert(aResponse);

  const std::lock_guard<std::mutex> myLock(theMutex);

  const auto it = theStateRepo.find(aState->name());
  if (it == theStateRepo.end()) {
    aResponse->set_retcode("could not find state: " + aState->name());
  } else {
    aResponse->mutable_state()->set_content(it->second);
    aResponse->set_retcode("OK");
  }
  return grpc::Status::OK;
}

grpc::Status StateServer::StateServerImpl::Put(
    [[maybe_unused]] grpc::ServerContext* aContext,
    const rpc::State*                     aState,
    rpc::StateResponse*                   aResponse) {
  assert(aState);
  assert(aResponse);

  const std::lock_guard<std::mutex> myLock(theMutex);
  theStateRepo[aState->name()] = aState->content();
  aResponse->set_retcode("OK");
  return grpc::Status::OK;
}

grpc::Status StateServer::StateServerImpl::Del(
    [[maybe_unused]] grpc::ServerContext* aContext,
    const rpc::State*                     aState,
    rpc::StateResponse*                   aResponse) {
  assert(aState);
  assert(aResponse);

  const std::lock_guard<std::mutex> myLock(theMutex);
  if (theStateRepo.erase(aState->name()) == 1) {
    aResponse->set_retcode("OK");
  } else {
    aResponse->set_retcode("state not found: " + aState->name());
  }
  return grpc::Status::OK;
}

StateServer::StateServer(const std::string& aEndpoint)
    : SimpleServer(aEndpoint)
    , theServerImpl() {
  LOG(INFO) << "Creating a state server at endpoint " << aEndpoint;
}

} // namespace edge
} // end namespace uiiit
