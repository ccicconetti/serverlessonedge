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

#pragma once

#include "Edge/edgeclientpool.h"
#include "Rest/server.h"
#include "Support/macros.h"

#include <string>

namespace uiiit {
namespace edge {

/**
 * A proxy that converts action invocation from an OpenWhisk client to
 * lambda function requests in uiiit::edge.
 */
class WskProxy final : public rest::Server
{
  NONCOPYABLE_NONMOVABLE(WskProxy);

 public:
  /**
   * \param aApiRoot the API root URL.
   *
   * \param aSecure if true use SSL/TLS authentication.
   *
   * \param aEndpoint the end-point of the edge server.
   *
   * \param aConcurrency the maximum number of clients per destination.
   *        0 means infinite.
   */
  explicit WskProxy(const std::string& aApiRoot,
                    const std::string& aEndpoint,
                    const bool         aSecure,
                    const size_t       aConcurrency);

 private:
  void handleInvocation(web::http::http_request aReq);

 private:
  const std::string theEndpoint;
  EdgeClientPool    theClientPool;
};

} // namespace edge
} // namespace uiiit
