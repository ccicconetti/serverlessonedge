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

#include "forwardingtableserver.h"

#include "forwardingtableinterface.h"

#include <glog/logging.h>
#include <grpc++/grpc++.h>

#include <cassert>

namespace uiiit {
namespace edge {

ForwardingTableServer::ForwardingTableServerImpl::ForwardingTableServerImpl(
    std::vector<ForwardingTableInterface*> aTables)
    : theTables(aTables) {
}

grpc::Status ForwardingTableServer::ForwardingTableServerImpl::Configure(
    grpc::ServerContext*       aContext,
    const rpc::EdgeRouterConf* aReq,
    rpc::Return*               aRep) {
  assert(aReq);
  assert(aRep);
  std::ignore = aContext;

  try {
    if (aReq->action() == uiiit::rpc::EdgeRouterConf::FLUSH) {
      // remove from all tables
      for (const auto myTable : theTables) {
        for (const auto& myLambda : myTable->lambdas()) {
          myTable->remove(myLambda);
        }
      }

    } else if (aReq->action() == uiiit::rpc::EdgeRouterConf::CHANGE) {
      assert(theTables.size() == 1 or theTables.size() == 2);
      // if there are two tables change the non-final destinations only in the
      // first one
      for (auto i = 0u; i < theTables.size(); i++) {
        if (theTables.size() == 1 or i == 0 or aReq->final()) {
          theTables[i]->change(aReq->lambda(),
                          aReq->destination(),
                          aReq->weight(),
                          aReq->final());
        }
      }

    } else if (aReq->action() == uiiit::rpc::EdgeRouterConf::REMOVE) {
      // remove from all tables
      for (const auto myTable : theTables) {
        myTable->remove(aReq->lambda(), aReq->destination());
      }

    } else {
      throw std::runtime_error(
          "Invalid action found in the configuration of a forwarding table");
    }

    if (VLOG_IS_ON(1)) {
      for (auto i = 0u; i < theTables.size(); i++) {
        LOG(INFO) << "New forwarding table#" << i << '\n' << *theTables[i];
      }
    }

    aRep->set_msg("OK");

  } catch (const std::exception& aErr) {
    aRep->set_msg(std::string("Error: ") + aErr.what());

  } catch (...) {
    aRep->set_msg("Unknown error");
  }

  return grpc::Status::OK;
}

grpc::Status ForwardingTableServer::ForwardingTableServerImpl::GetTable(
    grpc::ServerContext*  aContext,
    const rpc::TableId*   aReq,
    rpc::ForwardingTable* aRep) {
  assert(aReq);
  assert(aRep);
  std::ignore = aContext;

  const auto myTableId = aReq->value();

  // if a non-existing table is requested we just reply with an one
  if (myTableId < theTables.size()) {
    int myNdx = 0;
    for (const auto& myRow : theTables[myTableId]->fullTable()) {
      const auto& myLambda = myRow.first;
      for (const auto& myDestination : myRow.second) {
        aRep->add_entries();
        assert(aRep->entries_size() == (1 + myNdx));
        auto& myEntry = *aRep->mutable_entries(myNdx);

        myEntry.set_lambda(myLambda);
        myEntry.set_destination(myDestination.first);
        myEntry.set_weight(myDestination.second.first);
        myEntry.set_final(myDestination.second.second);
        ++myNdx;
      }
    }
    assert(aRep->entries_size() == myNdx);
  }

  return grpc::Status::OK;
}

grpc::Status ForwardingTableServer::ForwardingTableServerImpl::GetNumTables(
    grpc::ServerContext* aContext,
    const rpc::Void*     aReq,
    rpc::NumTables*      aRep) {
  assert(aReq);
  assert(aRep);
  std::ignore = aContext;

  aRep->set_value(theTables.size());

  return grpc::Status::OK;
}

ForwardingTableServer::ForwardingTableServer(const std::string& aServerEndpoint,
                                             ForwardingTableInterface& aTable)
    : ForwardingTableServer(aServerEndpoint, {&aTable}) {
  LOG(INFO) << "Creating a single forwarding table gRPC server";
}

ForwardingTableServer::ForwardingTableServer(
    const std::string&        aServerEndpoint,
    ForwardingTableInterface& aOverallTable,
    ForwardingTableInterface& aFinalTable)
    : ForwardingTableServer(aServerEndpoint, {&aOverallTable, &aFinalTable}) {
  LOG(INFO) << "Creating a dual forwarding table gRPC server:\n"
               "Table#0: all the destinations\n"
               "Table#1: only the final destinations";
}

ForwardingTableServer::ForwardingTableServer(
    const std::string&                     aServerEndpoint,
    std::vector<ForwardingTableInterface*> aTables)
    : SimpleServer(aServerEndpoint)
    , theServerImpl(aTables) {
  assert(not aTables.empty());
  for (const auto elem : aTables) {
    assert(elem != nullptr);
    std::ignore = elem;
  }
}

} // namespace edge
} // end namespace uiiit
