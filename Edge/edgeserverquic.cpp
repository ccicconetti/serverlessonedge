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

#include "edgeserverquic.h"

#include <glog/logging.h>

#include <boost/algorithm/string.hpp>

#include <folly/io/async/EventBaseManager.h>
#include <proxygen/httpserver/samples/hq/HQParams.h>
#include <quic/logging/FileQLogger.h>

namespace uiiit {
namespace edge {

using namespace std::chrono_literals;

proxygen::HTTPTransactionHandler*
Dispatcher::getRequestHandler(proxygen::HTTPMessage* msg,
                              const qs::HQParams&    params) {
  DCHECK(msg);
  // auto path = msg->getPathAsStringPiece();
  // if (path == "/" || path == "/echo") {
  //   return new qs::EchoHandler(params);
  // }
  // return new qs::DummyHandler(params);
  return new qs::EchoHandler(params);
}

/*
1) theMutex serve sempre in EdgeServerGrpc???

2) Valutare se è necessario lasciare tra i membri privati
theMutex, theServerEndpoint, theNumThreads
*/

EdgeServerQuic::EdgeServerQuic(EdgeServer&        aEdgeServer,
                               const qs::HQParams aQuicParamsConf)
    : EdgeServerImpl(aEdgeServer)
    , theMutex()
    , theServerEndpoint(aQuicParamsConf.host)
    , theNumThreads(aQuicParamsConf.httpServerThreads)
    //, theHandlers()
    , theQuicParamsConf(aQuicParamsConf)
    , theQuicTransportServer(theQuicParamsConf, Dispatcher::getRequestHandler) {
}

void EdgeServerQuic::run() {
  theH2ServerThread =
      H2Server::run(theQuicParamsConf, Dispatcher::getRequestHandler);

  theQuicServerThread = theQuicTransportServer.start();
}

void EdgeServerQuic::wait() { // wait for the server termination
  LOG(INFO) << "EdgeServerQuic::wait()\n";
}

EdgeServerQuic::~EdgeServerQuic() {
  // eventuali stop dei server
  LOG(INFO) << "EdgeServerQuic::DTOR()\n";
  theH2ServerThread.join();
  theQuicServerThread.join();
  LOG(INFO) << "POST EdgeServerQuic::DTOR()\n";
}

rpc::LambdaResponse EdgeServerQuic::process(const rpc::LambdaRequest& aReq) {
  LOG(INFO) << "EdgeServerQuic::PROCESS\n";
  return theEdgeServer.process(aReq);
}

std::set<std::thread::id> EdgeServerQuic::threadIds() const {
  std::set<std::thread::id> ret;
  ret.insert(theH2ServerThread.get_id());
  ret.insert(theQuicServerThread.get_id());
  return ret;
}

//**********************************************************************

} // namespace edge
} // namespace uiiit
