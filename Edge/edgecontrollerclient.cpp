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

#include "edgecontrollerclient.h"

#include "RpcSupport/utils.h"
#include "edgecontrollermessages.h"

#include <grpc++/grpc++.h>

#include <sstream>

namespace uiiit {
namespace edge {

EdgeControllerClient::EdgeControllerClient(const std::string& aServerEndpoint)
    : SimpleClient(aServerEndpoint) {
}

void EdgeControllerClient::announceComputer(
    const std::string& aEdgeServerEndpoint, const ContainerList& aContainers) {
  grpc::ClientContext myContext;
  rpc::Void           myRep;
  rpc::checkStatus(theStub->AnnounceComputer(
      &myContext, toProtobuf(aEdgeServerEndpoint, aContainers), &myRep));
}

void EdgeControllerClient::announceRouter(
    const std::string& aEdgeServerEndpoint,
    const std::string& aEdgeRouterEndpoint) {
  grpc::ClientContext myContext;
  rpc::Void           myRep;
  rpc::RouterInfo     myReq;
  myReq.set_edgeserverendpoint(aEdgeServerEndpoint);
  myReq.set_forwardingtableendpoint(aEdgeRouterEndpoint);
  rpc::checkStatus(theStub->AnnounceRouter(&myContext, myReq, &myRep));
}

void EdgeControllerClient::removeComputer(
    const std::string& aEdgeServerEndpoint) {
  grpc::ClientContext myContext;
  rpc::Void           myRep;
  rpc::checkStatus(theStub->RemoveComputer(
      &myContext, toProtobuf(aEdgeServerEndpoint, ContainerList()), &myRep));
}

} // end namespace edge
} // end namespace uiiit
