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

#include "Quic/curlclient.h"

namespace uiiit {
namespace edge {

CurlClient::CurlClient(folly::EventBase*            aEvb,
                       proxygen::HTTPMethod         aHttpMethod,
                       const proxygen::URL&         aUrl,
                       const proxygen::URL*         aProxy,
                       const proxygen::HTTPHeaders& aHeaders,
                       const std::string&           aInputFilename,
                       bool                         aH2c,
                       unsigned short               aHttpMajor,
                       unsigned short               aHttpMinor,
                       bool                         aPartiallyReliable)
    : CurlService::CurlClient(aEvb,
                              aHttpMethod,
                              aUrl,
                              aProxy,
                              aHeaders,
                              aInputFilename,
                              aH2c,
                              aHttpMajor,
                              aHttpMinor,
                              aPartiallyReliable) {
  VLOG(1) << "CurlClient::ctor";
}

void CurlClient::onBody(std::unique_ptr<folly::IOBuf> aChain) noexcept {
  VLOG(1) << "CurlClient::onBody";
  if (theResponseBodyChain) {
    theResponseBodyChain->prependChain(std::move(aChain));
  } else {
    theResponseBodyChain = std::move(aChain);
  }
}

folly::ByteRange CurlClient::getResponseBody() {
  VLOG(1) << "CurlClient::getResponseBody";
  return std::move(theResponseBody);
}

void CurlClient::onEOM() noexcept {
  VLOG(1) << "CurlClient::onEOM";
  theResponseBody = theResponseBodyChain->coalesce();
  evb_->terminateLoopSoon();
}

} // namespace edge
} // namespace uiiit