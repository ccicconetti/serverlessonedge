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

#include "edgecontrollerserver.h"

#include "forwardingtableclient.h"

#include <glog/logging.h>
#include <grpc++/grpc++.h>

#include <cassert>
#include <utility>

namespace uiiit {
namespace edge {

EdgeControllerServer::EdgeControllerServerImpl::EdgeControllerServerImpl(
    EdgeControllerServer& aServer)
    : theServer(aServer) {
}

grpc::Status EdgeControllerServer::EdgeControllerServerImpl::AnnounceComputer(
    grpc::ServerContext*     aContext,
    const rpc::ComputerInfo* aReq,
    rpc::Void*               aRep) {
  assert(aReq);
  assert(aRep);
  std::ignore = aContext;

  theServer.apply([&aReq](EdgeController& aController) {
    aController.announceComputer(aReq->edgeserverendpoint(),
                                 containerListFromProtobuf(*aReq));
  });
  return grpc::Status::OK;
}

grpc::Status EdgeControllerServer::EdgeControllerServerImpl::AnnounceRouter(
    grpc::ServerContext*   aContext,
    const rpc::RouterInfo* aReq,
    rpc::Void*             aRep) {
  assert(aReq);
  assert(aRep);
  std::ignore = aContext;

  theServer.apply([&aReq](EdgeController& aController) {
    aController.announceRouter(aReq->edgeserverendpoint(),
                               aReq->forwardingtableendpoint());
  });
  return grpc::Status::OK;
}

grpc::Status EdgeControllerServer::EdgeControllerServerImpl::RemoveComputer(
    grpc::ServerContext*     aContext,
    const rpc::ComputerInfo* aReq,
    rpc::Void*               aRep) {
  assert(aReq);
  assert(aRep);
  std::ignore = aContext;

  theServer.apply([&aReq](EdgeController& aController) {
    aController.removeComputer(aReq->edgeserverendpoint());
  });
  return grpc::Status::OK;
}

EdgeControllerServer::EdgeControllerServer(const std::string& aServerEndpoint)
    : SimpleServer(aServerEndpoint)
    , theControllers()
    , theServerImpl(*this) {
}

void EdgeControllerServer::subscribe(
    std::unique_ptr<EdgeController>&& aEdgeController) {
  theControllers.emplace_back(
      std::forward<std::unique_ptr<EdgeController>>(aEdgeController));
}

void EdgeControllerServer::apply(
    const std::function<void(EdgeController&)>& aHandler) noexcept {
  for (auto& myController : theControllers) {
    assert(myController != nullptr);
    try {
      aHandler(*myController);
    } catch (const std::exception& aErr) {
      LOG(WARNING) << "error when applying EdgeController method: "
                   << aErr.what();
    } catch (...) {
      LOG(WARNING) << "unknown error when applying EdgeController method";
    }
  }
}

} // namespace edge
} // end namespace uiiit
