/*
 ___ ___ __     __ ____________
|   |   |  |   |__|__|__   ___/  Ubiquitout Internet @ IIT-CNR
|   |   |  |  /__/  /  /  /      C++ ETSI MEC library
|   |   |  |/__/  /   /  /       https://github.com/ccicconetti/etsimec/
|_______|__|__/__/   /__/

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2019 Claudio Cicconetti https://ccicconetti.github.io/

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

#include "wskproxy.h"

#include <glog/logging.h>

namespace uiiit {
namespace edge {

WskProxy::WskProxy(const std::string& aApiRoot,
                   const std::string& aEndpoint,
                   const size_t       aConcurrency)
    : rest::Server(aApiRoot)
    , theEndpoint(aEndpoint)
    , theClientPool(aConcurrency) {
  (*this)(web::http::methods::POST,
          "/api/v1/namespaces/(.*)",
          [this](web::http::http_request aReq) { handleInvocation(aReq); });
}

void WskProxy::handleInvocation(web::http::http_request aReq) {
  VLOG(2) << "incoming request with path " << aReq.request_uri().path()
          << ", query " << aReq.request_uri().query();

  const auto myPath  = web::uri::split_path(aReq.request_uri().path());
  const auto myQuery = web::uri::split_query(aReq.request_uri().query());

  if (myPath.size() != 6 or myPath[0] != "api" or myPath[1] != "v1" or
      myPath[2] != "namespaces" or myPath[5].empty()) {
    LOG(WARNING) << "invalid path requested: " << aReq.request_uri().path();
    aReq.reply(web::http::status_codes::BadRequest);

  } else if (myQuery.count("blocking") == 0 or myQuery.count("result") == 0) {
    LOG(WARNING) << "invalid query, missing blocking or result field: "
                 << aReq.request_uri().query();
    aReq.reply(web::http::status_codes::BadRequest);
  } else {
    if (myQuery.at("blocking") != "true" or myQuery.at("result") != "true" or
        myPath[4] != "actions") {
      aReq.reply(web::http::status_codes::NotImplemented);

    } else {
      // determine the lambda name, which can be:
      // 1. NAME, or
      // 2. /NAMESPACE/NAME
      auto myLambdaName = myPath[5]; // no namespace
      if (myPath[3] != "_") {
        myLambdaName = std::string("/") + myPath[3] + "/" + myPath[5];
      }

      // extract body
      std::string myBody;
      aReq.extract_string()
          .then([&myBody](pplx::task<std::string> aPrevTask) {
            myBody = aPrevTask.get();
          })
          .wait();

      const auto res = theClientPool(
          theEndpoint, LambdaRequest(myLambdaName, myBody), false);
      VLOG(1) << res.second << ' ' << res.first;

      if (res.first.theRetCode != "OK") {
        aReq.reply(web::http::status_codes::NotFound,
                   std::string("{\"error\":\"") + res.first.theRetCode + "\"}");
      } else {
        aReq.reply(web::http::status_codes::OK,
                   std::string("{\"payload\":\"") + res.first.theOutput + "\"}");
      }
    }
  }
}

} // namespace edge
} // namespace uiiit
