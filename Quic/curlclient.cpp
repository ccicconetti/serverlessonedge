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

CurlClient::CurlClient(folly::EventBase*            evb,
                       proxygen::HTTPMethod         httpMethod,
                       const proxygen::URL&         url,
                       const proxygen::URL*         proxy,
                       const proxygen::HTTPHeaders& headers,
                       const std::string&           inputFilename,
                       bool                         h2c,
                       unsigned short               httpMajor,
                       unsigned short               httpMinor,
                       bool                         partiallyReliable)
    : CurlService::CurlClient(evb,
                              httpMethod,
                              url,
                              proxy,
                              headers,
                              inputFilename,
                              h2c,
                              httpMajor,
                              httpMinor,
                              partiallyReliable) {
}

void CurlClient::onBody(std::unique_ptr<folly::IOBuf> chain) noexcept {
  LOG(INFO) << "CurlClient::onBody()";
  theResponseBody = std::move(chain);
}

std::unique_ptr<folly::IOBuf> CurlClient::getResponseBody() {
  LOG(INFO) << "CurlClient::getResponseBody()";
  return std::move(theResponseBody);
}

} // namespace edge
} // namespace uiiit