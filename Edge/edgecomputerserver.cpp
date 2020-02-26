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

#include "edgecomputerserver.h"

namespace uiiit {
namespace edge {

EdgeComputerServer::EdgeComputerServerImpl::EdgeComputerServerImpl(
    EdgeComputerServer& aParent)
    : SimpleStreamingServerImpl(aParent) {
}

EdgeComputerServer::EdgeComputerServerImpl::~EdgeComputerServerImpl() {
}

grpc::Status EdgeComputerServer::EdgeComputerServerImpl::StreamUtil(
    grpc::ServerContext*                  aContext,
    const rpc::Void*                      aReq,
    grpc::ServerWriter<rpc::Utilization>* aWriter) {
  return handle(aContext, [&aWriter](const rpc::Utilization& aMsg) {
    return aWriter->Write(aMsg);
  });
}

EdgeComputerServer::EdgeComputerServer(const std::string& aServerEndpoint)
    : SimpleStreamingServer(aServerEndpoint)
    , theServerImpl(*this) {
}

void EdgeComputerServer::add(const std::map<std::string, double>& aUtil) {
  auto myMsg = std::make_shared<rpc::Utilization>();
  for (const auto& myPair : aUtil) {
    (*myMsg->mutable_values())[myPair.first] = myPair.second;
  }
  push(myMsg);
}

} // namespace edge
} // end namespace uiiit
