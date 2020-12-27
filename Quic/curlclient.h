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

#pragma once

#include <proxygen/httpclient/samples/curl/CurlClient.h>

namespace uiiit {
namespace edge {

/**
 * This class is needed because the CurlClient sample in Proxygen library
 * has not a protected member useful to store the HTTPResponse body returned by
 * the LambdaRequestHandler (the getResponse method returns a
 * proxygen::HTTPMessage that is defined as the message without its body).
 * We need to extend it to provide a member variable in which we can save the
 * body of the response and the related getter method (getResponseBody)
 * The latter returns a folly::IOBUF for the purpose of generality for
 * eventual future extensions not related to LambdaRequest/Response
 */
class CurlClient final : public CurlService::CurlClient
{
 public:
  explicit CurlClient(folly::EventBase*            evb,
                      proxygen::HTTPMethod         httpMethod,
                      const proxygen::URL&         url,
                      const proxygen::URL*         proxy,
                      const proxygen::HTTPHeaders& headers,
                      const std::string&           inputFilename,
                      bool                         h2c               = false,
                      unsigned short               httpMajor         = 1,
                      unsigned short               httpMinor         = 1,
                      bool                         partiallyReliable = false);

  virtual ~CurlClient() = default;

  void onBody(std::unique_ptr<folly::IOBuf> chain) noexcept override;
  std::unique_ptr<folly::IOBuf> getResponseBody();

 protected:
  std::unique_ptr<folly::IOBuf> theResponseBody;
};

} // namespace edge
} // namespace uiiit