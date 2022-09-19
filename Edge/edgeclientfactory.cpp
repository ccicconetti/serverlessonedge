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

#include "edgeclientfactory.h"

#include "Edge/edgeclientgrpc.h"
#include "Edge/edgeclientinterface.h"
#include "Edge/edgeclientmulti.h"
#include "Support/conf.h"

#ifdef WITH_QUIC
#include "Quic/edgeclientquic.h"
#include "Quic/quicparamsbuilder.h"
#endif

#include <stdexcept>

namespace uiiit {
namespace edge {

std::unique_ptr<EdgeClientInterface>
EdgeClientFactory::make(const std::set<std::string>& aEndpoints,
                        const bool                   aSecure,
                        const support::Conf&         aConf) {
  if (aEndpoints.empty()) {
    throw std::runtime_error(
        "Cannot create an edge client with an empty set of destinations");
  }

  const auto myType = aConf("type");
  if (aEndpoints.size() == 1) {
    if (myType == "grpc") {
      return std::make_unique<EdgeClientGrpc>(*aEndpoints.begin(), aSecure);
#ifdef WITH_QUIC
    } else if (myType == "quic") {
      return std::make_unique<EdgeClientQuic>(
          QuicParamsBuilder::buildClientHQParams(aConf, *aEndpoints.begin()));
#endif
    } else {
      throw std::runtime_error("Unknown client type: " + myType);
    }
  }

  return std::make_unique<EdgeClientMulti>(aEndpoints, aSecure, aConf);
}

} // end namespace edge
} // end namespace uiiit
