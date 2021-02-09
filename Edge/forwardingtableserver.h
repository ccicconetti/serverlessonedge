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

#include "RpcSupport/simpleserver.h"

#include <string>
#include <vector>

#include "edgerouter.grpc.pb.h"

namespace uiiit {
namespace edge {

class ForwardingTableInterface;

class ForwardingTableServer final : public rpc::SimpleServer
{
  class ForwardingTableServerImpl final : public rpc::EdgeRouter::Service
  {
   public:
    explicit ForwardingTableServerImpl(
        std::vector<ForwardingTableInterface*> aTables);

   private:
    grpc::Status Configure(grpc::ServerContext*       aContext,
                           const rpc::EdgeRouterConf* aReq,
                           rpc::Return*               aRep) override;
    grpc::Status GetTable(grpc::ServerContext*  aContext,
                          const rpc::TableId*   aReq,
                          rpc::ForwardingTable* aRep) override;
    grpc::Status GetNumTables(grpc::ServerContext* aContext,
                              const rpc::Void*     aReq,
                              rpc::NumTables*      aRep) override;

    std::vector<ForwardingTableInterface*> theTables;
  };

 public:
  /**
   * Create a gRPC forwarding table interface for a single table.
   */
  explicit ForwardingTableServer(const std::string&        aServerEndpoint,
                                 ForwardingTableInterface& aTable);

  /**
   * Create a gRPC forwarding table interface for two tables.
   *
   * \param aOverallTable The table containing all types of destinations.
   * \param aFinalTable The table containing only the final destinations.
   */
  explicit ForwardingTableServer(const std::string&        aServerEndpoint,
                                 ForwardingTableInterface& aOverallTable,
                                 ForwardingTableInterface& aFinalTable);

 private:
  /**
   * Create a gRPC server acting as an interface for an array of forwarding
   * tables.
   *
   * \param aServerEndpoint the gRPC used by clients to connect.
   * \param aTables the array of forwarding table pointers. Note the use of raw
   *        pointers: they cannot be null and the pointed objects must survive
   *        the corresponding ForwardingTableServer instances.
   */
  explicit ForwardingTableServer(
      const std::string&                     aServerEndpoint,
      std::vector<ForwardingTableInterface*> aTables);

  grpc::Service& service() override {
    return theServerImpl;
  }

 private:
  ForwardingTableServerImpl theServerImpl;
};

} // end namespace edge
} // end namespace uiiit
