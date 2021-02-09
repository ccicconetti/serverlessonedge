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

#include "forwardingtableclient.h"

#include "Edge/Detail/printtable.h"
#include "RpcSupport/utils.h"
#include "forwardingtable.h"

#include <grpc++/grpc++.h>

#include <sstream>

namespace uiiit {
namespace edge {

ForwardingTableClient::ForwardingTableClient(const std::string& aServerEndpoint)
    : SimpleClient(aServerEndpoint) {
}

size_t ForwardingTableClient::numTables() {
  grpc::ClientContext myContext;
  rpc::Void           myReq;
  rpc::NumTables      myRep;
  rpc::checkStatus(theStub->GetNumTables(&myContext, myReq, &myRep));
  return myRep.value();
}

std::map<std::string, std::map<std::string, std::pair<float, bool>>>
ForwardingTableClient::table(const size_t aTableId) {
  grpc::ClientContext myContext;
  rpc::TableId        myReq;
  myReq.set_value(aTableId);
  rpc::ForwardingTable myRep;
  rpc::checkStatus(theStub->GetTable(&myContext, myReq, &myRep));

  std::map<std::string, std::map<std::string, std::pair<float, bool>>> myTable;
  for (auto i = 0; i < myRep.entries_size(); i++) {
    const auto& myLambda      = myRep.entries(i).lambda();
    const auto& myDestination = myRep.entries(i).destination();
    const auto& myWeight      = myRep.entries(i).weight();
    const auto& myFinal       = myRep.entries(i).final();

    const auto myPair = myTable.insert(
        {myLambda, {{myDestination, std::make_pair(myWeight, myFinal)}}});
    if (not myPair.second) {
      myPair.first->second.insert(
          {myDestination, std::make_pair(myWeight, myFinal)});
    }
  }

  return myTable;
}

std::string ForwardingTableClient::dump() {
  std::stringstream myBuf;
  for (auto i = 0u; i < numTables(); i++) {
    myBuf << "Table#" << i << '\n';
    detail::printTable(myBuf, table(i));
  }
  return myBuf.str();
}

void ForwardingTableClient::flush() {
  rpc::EdgeRouterConf myReq;
  myReq.set_action(rpc::EdgeRouterConf::FLUSH);
  // all other fields not set
  send(myReq);
}

void ForwardingTableClient::change(const std::string& aLambda,
                                   const std::string& aDestination,
                                   const float        aWeight,
                                   const bool         aFinal) {
  rpc::EdgeRouterConf myReq;
  myReq.set_action(rpc::EdgeRouterConf::CHANGE);
  myReq.set_lambda(aLambda);
  myReq.set_destination(aDestination);
  myReq.set_weight(aWeight);
  myReq.set_final(aFinal);
  send(myReq);
}

void ForwardingTableClient::remove(const std::string& aLambda,
                                   const std::string& aDestination) {
  rpc::EdgeRouterConf myReq;
  myReq.set_action(rpc::EdgeRouterConf::REMOVE);
  myReq.set_lambda(aLambda);
  myReq.set_destination(aDestination);
  // weight and final not set
  send(myReq);
}

void ForwardingTableClient::send(const rpc::EdgeRouterConf& aReq) {
  grpc::ClientContext myContext;
  rpc::Return         myRep;
  rpc::checkStatus(theStub->Configure(&myContext, aReq, &myRep));
}

} // end namespace edge
} // end namespace uiiit
