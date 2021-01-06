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

#include "edgeclientfactory.h"

#include "Edge/edgeclientgrpc.h"
#include "Edge/edgeclientinterface.h"
#include "Edge/edgeclientmulti.h"
#include "Quic/edgeclientquic.h"
#include "Quic/quicparamsbuilder.h"
#include "Support/conf.h"

#include <stdexcept>

namespace uiiit {
namespace edge {

std::unique_ptr<EdgeClientInterface>
EdgeClientFactory::make(const std::set<std::string>& aEndpoints,
                        const support::Conf&         aConf) {
  if (aEndpoints.empty()) {
    throw std::runtime_error(
        "Cannot create an edge client with an empty set of destinations");
  }

  const auto myType = aConf("transport-type");
  if (aEndpoints.size() == 1) {
    if (myType == std::string("grpc")) {
      return std::make_unique<EdgeClientGrpc>(*aEndpoints.begin());
    } else { // transport-type = quic
      return std::make_unique<EdgeClientQuic>(
          QuicParamsBuilder::build(aConf, *aEndpoints.begin(), false));
    }
  }

  return std::make_unique<EdgeClientMulti>(aEndpoints, aConf);
}

} // end namespace edge
} // end namespace uiiit
