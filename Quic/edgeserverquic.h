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

#pragma once

#include "Edge/edgeserverimpl.h"
#include "Quic/h2server.h"
#include "Quic/hqserver.h"
#include "Quic/hqservertransportfactory.h"
#include "Quic/hqsessioncontroller.h"
#include "Quic/quicparams.h"
#include "Support/macros.h"

//#include <proxygen/httpserver/samples/hq/HQParams.h>

#include <cassert>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>

namespace uiiit {
namespace edge {

struct LambdaResponse;

using HTTPTransactionHandlerProvider =
    std::function<proxygen::HTTPTransactionHandler*(proxygen::HTTPMessage*,
                                                    const HQParams&)>;
// proxygen::HTTPMessage*, const quic::samples::HQParams&)>;

class Dispatcher
{
 public:
  static proxygen::HTTPTransactionHandler*
  getRequestHandler(proxygen::HTTPMessage* /* msg */,
                    const HQParams& /* params */);
  // const quic::samples::HQParams& /* params */);
};

/**
 * Generic edge server providing a multi-threaded QUIC server interface for the
 * processing of lambda functions.
 */
class EdgeServerQuic final : public EdgeServerImpl
{

 public:
  NONCOPYABLE_NONMOVABLE(EdgeServerQuic);

  //! Create an edge server with a given number of threads.
  explicit EdgeServerQuic(EdgeServer& aEdgeServer,
                          // const quic::samples::HQParams aQuicParamsConf);
                          const HQParams aQuicParamsConf);

  virtual ~EdgeServerQuic();

  //! Start the server. No more configuration allowed after this call.
  void run() override;

  //! Wait until termination of the server.
  void wait() override;

 protected:
  /**
   * \return the set of the identifiers of the threads that have been
   * spawned during the call to run(). The cardinality of this set
   * if equal to the number of threads specified in the ctor. If
   * run() has not (yet) been called, then an empty set is returned.
   */
  std::set<std::thread::id> threadIds() const;

 private:
  //! Thread execution body.
  void handle();

  //! Perform actual processing of a lambda request.
  rpc::LambdaResponse process(const rpc::LambdaRequest& aReq) override;

 protected:
  mutable std::mutex theMutex;
  const std::string  theServerEndpoint;
  const size_t       theNumThreads;

 private:
  // std::list<std::thread> theHandlers;
  std::thread theH2ServerThread;
  std::thread theQuicServerThread;
  // const quic::samples::HQParams theQuicParamsConf;
  const HQParams theQuicParamsConf;
  HQServer       theQuicTransportServer;

}; // end class EdgeServer

} // namespace edge
} // end namespace uiiit