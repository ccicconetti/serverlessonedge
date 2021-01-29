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

HQParams
QuicParamsBuilder::buildClientHQParams(const support::Conf& aConf,
                                       const std::string&   aServerEndpoint) {
  VLOG(4) << "HQParamsBuilder::buildClientHQParams";

  HQParams myHQParamsConf;

  // aServerEndpoint format is "IPaddress:port"
  const auto myServerIPPortVector =
      support::split<std::vector<std::string>>(aServerEndpoint, ":");

  // *** Common Settings Section ***
  myHQParamsConf.host = myServerIPPortVector[0];
  myHQParamsConf.port = std::stoi(myServerIPPortVector[1]); // possible overflow

  myHQParamsConf.remoteAddress =
      folly::SocketAddress(myHQParamsConf.host, myHQParamsConf.port, true);

  myHQParamsConf.transportSettings.shouldDrain = false;

  // (Client Only) attempt early-data (bool) -> "whether to use 0-RTT"
  if (aConf.find(std::string("attempt-early-data")) != aConf.end()) {
    myHQParamsConf.transportSettings.attemptEarlyData =
        aConf.getBool("attempt-early-data");
    VLOG(4) << "attempt-early-data in Conf = "
            << myHQParamsConf.transportSettings.attemptEarlyData;
    myHQParamsConf.earlyData = aConf.getBool("attempt-early-data");
  } else {
    myHQParamsConf.transportSettings.attemptEarlyData = false;
    VLOG(4) << "attempt-early-data default = "
            << myHQParamsConf.transportSettings.attemptEarlyData;
    myHQParamsConf.earlyData = false;
  }

  return myHQParamsConf;
}

HQParams
QuicParamsBuilder::buildServerHQParams(const support::Conf& aConf,
                                       const std::string&   aServerEndpoint,
                                       const size_t         aNumThreads) {
  VLOG(4) << "HQParamsBuilder::buildServerHQParams";
  HQParams myHQParamsConf;

  // aServerEndpoint format is "IPaddress:port"
  const auto myServerIPPortVector =
      support::split<std::vector<std::string>>(aServerEndpoint, ":");

  // *** Common Settings Section ***
  myHQParamsConf.host = myServerIPPortVector[0];
  myHQParamsConf.port = std::stoi(myServerIPPortVector[1]); // possible overflow

  myHQParamsConf.localAddress =
      folly::SocketAddress(myHQParamsConf.host, myHQParamsConf.port, true);

  // (Server Only) httpServerThreads (size_t)
  myHQParamsConf.httpServerThreads = aNumThreads;

  return myHQParamsConf;
} // namespace edge

} // namespace edge
} // namespace uiiit