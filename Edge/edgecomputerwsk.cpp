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

#include "edgecomputerwsk.h"

#include "Support/split.h"
#include "edgemessages.h"

#include <glog/logging.h>

#include <grpc++/grpc++.h>

namespace uiiit {
namespace edge {

EdgeComputerWsk::EdgeComputerWsk(const std::string& aServerEndpoint,
                                 const std::string& aWskApiRoot,
                                 const std::string& aWskAuth)
    : EdgeServer(aServerEndpoint) //aNumThreads
    , theWskInvoker(aWskApiRoot, aWskAuth) {
  // noop
}

rpc::LambdaResponse EdgeComputerWsk::process(const rpc::LambdaRequest& aReq) {
  rpc::LambdaResponse myResp;

  std::string myRetCode = "OK";
  try {
    if (not aReq.dry()) {
      // determine the name + namespace from the lambda name
      const auto myTokens =
          support::split<std::vector<std::string>>(aReq.name(), "/");

      if (myTokens.empty()) {
        throw std::runtime_error(
            "cannot request execution of an action with empty name");
      }

      if (myTokens.size() > 2) {
        // unknown format, we don't expect more than one nesting of namespaces
        throw std::runtime_error(
            "invalid OpenWhisk action /namespace/name structure: " +
            aReq.name());
      }

      const auto& myName  = myTokens.size() == 1 ? myTokens[0] : myTokens[1];
      const auto  mySpace = myTokens.size() == 1 ? std::string() : myTokens[0];

      // this blocks until a response is given, errors must be checked
      // on the return value (no exception should be thrown): if the
      // first element of the pair is false, then the second element
      // contains a string with the description of the error;
      // otherwise, it contains the actual response
      const auto res = theWskInvoker(myName, mySpace, aReq.input());

      if (res.first) {
        VLOG(2) << "valid response to OpenWhisk action " << myName
                << " namespace " << (mySpace.empty() ? "_" : mySpace)
                << " invocation at " << theWskInvoker.apiRoot() << "\n"
                << "input: " << aReq.input() << "\n"
                << "output: " << res.second;

        myResp.set_output(res.second);

      } else {
        VLOG(2) << "invalid response to OpenWhisk action " << myName
                << " namespace " << (mySpace.empty() ? "_" : mySpace)
                << " invocation at " << theWskInvoker.apiRoot() << ": "
                << res.second;
        myRetCode = res.second;
      }
    }

  } catch (const std::exception& aErr) {
    myRetCode = aErr.what();
  } catch (...) {
    myRetCode = "Unknown error";
  }

  myResp.set_hops(aReq.hops() + 1);
  myResp.set_retcode(myRetCode);
  myResp.set_responder(theWskInvoker.apiRoot());
  return myResp;
}

} // namespace edge
} // namespace uiiit
