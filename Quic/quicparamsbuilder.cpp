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

HQParams QuicParamsBuilder::build(const support::Conf& aConf,
                                  const std::string&   aServerEndpoint,
                                  bool                 isServer) {
  HQParams myHQParamsConf(aServerEndpoint, isServer);

  // h2serverport (uint16_t)
  if (aConf.find(std::string("h2port")) != aConf.end()) {
    myHQParamsConf.h2port = aConf.getUint("h2port");
    VLOG(10) << "h2port in Conf = " << myHQParamsConf.h2port;
  } else {
    myHQParamsConf.h2port = 6667;
    VLOG(10) << "h2port default = " << myHQParamsConf.h2port;
  }

  myHQParamsConf.localH2Address =
      folly::SocketAddress(myHQParamsConf.host, myHQParamsConf.h2port, true);

  // (Server Only) httpServerThreads (size_t)
  if (isServer) {
    if (aConf.find(std::string("httpServerThreads")) != aConf.end()) {
      myHQParamsConf.httpServerThreads = aConf.getUint("httpServerThreads");
      LOG(INFO) << "httpServerThreads in Conf = "
                << myHQParamsConf.httpServerThreads;
    } else {
      myHQParamsConf.httpServerThreads = 5;
      LOG(INFO) << "httpServerThreads default = "
                << myHQParamsConf.httpServerThreads;
    }
  }

  // (Client Only) attempt early-data (bool) -> "whether to use 0-RTT"
  if (!isServer) {
    if (aConf.find(std::string("attempt-early-data")) != aConf.end()) {
      myHQParamsConf.transportSettings.attemptEarlyData =
          aConf.getBool("attempt-early-data");
      VLOG(10) << "attempt-early-data in Conf = "
               << myHQParamsConf.transportSettings.attemptEarlyData;
      myHQParamsConf.earlyData = aConf.getBool("attempt-early-data");
    } else {
      myHQParamsConf.transportSettings.attemptEarlyData = false;
      VLOG(10) << "attempt-early-data default = "
               << myHQParamsConf.transportSettings.attemptEarlyData;
      myHQParamsConf.earlyData = false;
    }
  }

  return myHQParamsConf;
}

} // namespace edge
} // namespace uiiit