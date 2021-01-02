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

#include "Quic/quicparamsbuilder.h"

namespace uiiit {
namespace edge {

HQParams QuicParamsBuilder::build(const support::Conf& aConf, bool isServer) {
  HQParams myHQParamsConf(isServer);

  // host (std::string)
  if (aConf.find(std::string("host")) != aConf.end()) {
    myHQParamsConf.host = aConf(std::string("host"));
    LOG(INFO) << "host in Conf = " << myHQParamsConf.host;
  } else {
    myHQParamsConf.host = std::string("127.0.0.1");
    LOG(INFO) << "host default = " << myHQParamsConf.host;
  }

  // port (uint16_t)
  if (aConf.find(std::string("port")) != aConf.end()) {
    myHQParamsConf.port = aConf.getUint("port");
    LOG(INFO) << "port in Conf = " << myHQParamsConf.port;
  } else {
    myHQParamsConf.port = 6473;
    LOG(INFO) << "port default = " << myHQParamsConf.port;
  }

  // Socket configuration according parameters passed by CLI
  if (isServer) {
    myHQParamsConf.localAddress =
        folly::SocketAddress(myHQParamsConf.host, myHQParamsConf.port, true);
  } else {
    myHQParamsConf.remoteAddress =
        folly::SocketAddress(myHQParamsConf.host, myHQParamsConf.port, true);
    // local_address empty by default (only for Client), local_address not set
    // CHECK if this local_address is needed in edgeclient(grpc)
  }

  // h2serverport (uint16_t)
  if (aConf.find(std::string("h2port")) != aConf.end()) {
    myHQParamsConf.h2port = aConf.getUint("h2port");
    LOG(INFO) << "h2port in Conf = " << myHQParamsConf.h2port;
  } else {
    myHQParamsConf.h2port = 6667;
    LOG(INFO) << "h2port default = " << myHQParamsConf.h2port;
  }

  myHQParamsConf.localH2Address =
      folly::SocketAddress(myHQParamsConf.host, myHQParamsConf.h2port, true);

  // attempt early-data (bool) -> "whether to use 0-RTT"
  if (aConf.find(std::string("attempt-early-data")) != aConf.end()) {
    if (!isServer) {
      myHQParamsConf.transportSettings.attemptEarlyData =
          aConf.getBool("attempt-early-data");
      LOG(INFO) << "attempt-early-data in Conf = "
                << myHQParamsConf.transportSettings.attemptEarlyData;
    }
    myHQParamsConf.earlyData = aConf.getBool("attempt-early-data");
  } else {
    myHQParamsConf.transportSettings.attemptEarlyData = false;
    LOG(INFO) << "attempt-early-data default = "
              << myHQParamsConf.transportSettings.attemptEarlyData;
    myHQParamsConf.earlyData = false;
  }

  return myHQParamsConf;

  // cascade of if to see if parameters are set by cli and eventually set the
  // realted HQParams parameter value
}

} // namespace edge
} // namespace uiiit