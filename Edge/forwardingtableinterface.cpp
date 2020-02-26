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

#include "forwardingtableinterface.h"

#include "Detail/printtable.h"

#include <glog/logging.h>

namespace uiiit {
namespace edge {

void fakeFill(ForwardingTableInterface& aTable,
              const size_t              aNumLambdas,
              const size_t              aNumDestinations) {
  for (size_t i = 0; i < aNumLambdas; i++) {
    const auto myLambda = std::string("lambda") + std::to_string(i);
    for (size_t j = 0; j < aNumDestinations; j++) {
      if (j % 100 == 0) {
        LOG(INFO) << "lambdas " << (1 + i) << "/" << aNumLambdas
                  << " destinations " << (1 + j) << "/" << aNumDestinations;
      }

      const auto myDestination = std::string("127.0.0.1:") + std::to_string(j);
      LOG_IF(WARNING, j > 65535) << "Invalid end-point: " << myDestination;

      aTable.change(myLambda, myDestination, 1.0f, true);
    }
  }
}

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                                aStream,
                         const uiiit::edge::ForwardingTableInterface& aTable) {
  uiiit::edge::detail::printTable(aStream, aTable.fullTable());
  return aStream;
}
