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
#include "RpcSupport/simpleserver.h"

#include <functional>
#include <list>
#include <memory>
#include <string>

#include "edgecontroller.grpc.pb.h"

namespace uiiit {
namespace edge {

/**
 * Create a gRPC server to collect announcements from edge routers/computers and
 * react accordingly via gRPC on edge routers to adapt their forwarding tables.
 *
 * It is possible to register further EdgeControllers to take external actions.
 */
class EdgeControllerServer final : public rpc::SimpleServer
{
  class EdgeControllerServerImpl final : public rpc::EdgeController::Service
  {
   public:
    explicit EdgeControllerServerImpl(EdgeControllerServer& aServer);

   private:
    grpc::Status AnnounceComputer(grpc::ServerContext*     aContext,
                                  const rpc::ComputerInfo* aReq,
                                  rpc::Void*               aRep) override;
    grpc::Status AnnounceRouter(grpc::ServerContext*   aContext,
                                const rpc::RouterInfo* aReq,
                                rpc::Void*             aRep) override;
    grpc::Status RemoveComputer(grpc::ServerContext*     aContext,
                                const rpc::ComputerInfo* aReq,
                                rpc::Void*               aRep) override;

   private:
    EdgeControllerServer& theServer;
  };

 public:
  explicit EdgeControllerServer(const std::string& aServerEndpoint);

  void subscribe(std::unique_ptr<EdgeController>&& aEdgeController);

 private:
  grpc::Service& service() override {
    return theServerImpl;
  }

  //! Apply the given function to all controllers.
  void apply(const std::function<void(EdgeController&)>& aHandler) noexcept;

 private:
  std::list<std::unique_ptr<EdgeController>> theControllers;
  EdgeControllerServerImpl                   theServerImpl;
};

} // end namespace edge
} // end namespace uiiit
