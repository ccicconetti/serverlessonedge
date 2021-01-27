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

#include "Quic/edgeserverquic.h"
#include "Quic/echohandler.h"
#include "Quic/lambdarequesthandler.h"

#include <boost/algorithm/string.hpp>
#include <folly/io/async/EventBaseManager.h>
#include <quic/logging/FileQLogger.h>

#include <glog/logging.h>

namespace uiiit {
namespace edge {

using namespace std::chrono_literals;

EdgeServerQuic::EdgeServerQuic(EdgeServer&     aEdgeServer,
                               const HQParams& aQuicParamsConf)
    : EdgeServerImpl(aEdgeServer)
    , theMutex()
    , theServerEndpoint(aQuicParamsConf.host)
    , theNumThreads(aQuicParamsConf.httpServerThreads)
    , theQuicParamsConf(aQuicParamsConf)
    , theQuicTransportServer(
          theQuicParamsConf,
          [this](proxygen::HTTPMessage* aMsg,
                 const HQParams& aParams) -> proxygen::HTTPTransactionHandler* {
            auto path = aMsg->getPathAsStringPiece();
            if (path == "/lambda") {
              return new LambdaRequestHandler(
                  aParams,
                  *this,
                  theQuicParamsConf.host + ':' +
                      std::to_string(theQuicParamsConf.port));
            }
            return new EchoHandler(aParams);
          }) {
  VLOG(4) << "EdgeServerQuic::ctor";
  theEdgeServer.init();
} // namespace edge

void EdgeServerQuic::run() {
  VLOG(4) << "EdgeServerQuic::run";
  theQuicTransportServer.start(
      theQuicParamsConf.httpServerThreads); // blocking until server initialized
  theQuicServerThread =
      std::thread([this]() mutable { theQuicTransportServer.run(); });
}

void EdgeServerQuic::wait() {
  VLOG(4) << "EdgeServerQuic::wait()\n";
  theQuicServerThread.join();
}

EdgeServerQuic::~EdgeServerQuic() {
  VLOG(1) << "EdgeServerQuic::dtor()\n";
  theQuicTransportServer.stop();

  if (theQuicServerThread.joinable())
    theQuicServerThread.join();
}

rpc::LambdaResponse EdgeServerQuic::process(const rpc::LambdaRequest& aReq) {
  VLOG(4) << "EdgeServerQuic::process\n";
  return theEdgeServer.process(aReq);
}

std::set<std::thread::id> EdgeServerQuic::threadIds() const {
  VLOG(4) << "EdgeServerQuic::threadIds";
  std::set<std::thread::id> ret;
  ret.insert(theQuicServerThread.get_id());
  return ret;
}

} // namespace edge
} // namespace uiiit
