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

#include "Edge/stateclient.h"

#include "RpcSupport/utils.h"

#include <glog/logging.h>
#include <grpc++/grpc++.h>

#include <utility>

namespace uiiit {
namespace edge {

StateClient::StateClient(const std::string& aServerEndpoint)
    : SimpleClient(aServerEndpoint) {
  // nihil
}

bool StateClient::Get(const std::string& aName, std::string& aState) {
  rpc::State myRequest;
  myRequest.set_name(aName);
  rpc::StateResponse                   myResponse;
  [[maybe_unused]] grpc::ClientContext myContext;

  rpc::checkStatus(theStub->Get(&myContext, myRequest, &myResponse));

  if (myResponse.retcode() != "OK") {
    LOG(ERROR) << "error when retrieving state " << aName << " from "
               << serverEndpoint() << ": " << myResponse.retcode();
    return false;
  }
  std::swap(aState, *myResponse.mutable_state()->mutable_content());
  return true;
}

void StateClient::Put(const std::string& aName, const std::string& aState) {
  rpc::State myRequest;
  myRequest.set_name(aName);
  myRequest.set_content(aState);
  rpc::StateResponse                   myResponse;
  [[maybe_unused]] grpc::ClientContext myContext;

  rpc::checkStatus(theStub->Put(&myContext, myRequest, &myResponse));

  if (myResponse.retcode() != "OK") {
    LOG(ERROR) << "error when updating state " << aName << " on "
               << serverEndpoint() << ": " << myResponse.retcode();
  }
}

bool StateClient::Del(const std::string& aName) {
  rpc::State myRequest;
  myRequest.set_name(aName);
  rpc::StateResponse                   myResponse;
  [[maybe_unused]] grpc::ClientContext myContext;

  rpc::checkStatus(theStub->Del(&myContext, myRequest, &myResponse));

  if (myResponse.retcode() != "OK") {
    LOG(ERROR) << "error when deleting state " << aName << " from "
               << serverEndpoint() << ": " << myResponse.retcode();
    return false;
  }
  return true;
}

} // namespace edge
} // namespace uiiit
