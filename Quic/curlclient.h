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

#include <folly/io/async/EventBase.h>
#include <folly/io/async/SSLContext.h>
#include <fstream>
#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>
#include <proxygen/lib/utils/URL.h>

namespace uiiit {
namespace edge {

// : public CurlService::CurlClient
class CurlClient final : public proxygen::HTTPConnector::Callback,
                         public proxygen::HTTPTransactionHandler
{
 public:
  explicit CurlClient(folly::EventBase*            aEvb,
                      proxygen::HTTPMethod         aHttpMethod,
                      const proxygen::URL&         aUrl,
                      const proxygen::HTTPHeaders& aHeaders,
                      bool                         aH2c               = false,
                      unsigned short               aHttpMajor         = 1,
                      unsigned short               aHttpMinor         = 1,
                      bool                         aPartiallyReliable = false);

  virtual ~CurlClient() = default;

  static proxygen::HTTPHeaders parseHeaders(const std::string& headersString);

  // HTTPConnector methods
  void connectSuccess(proxygen::HTTPUpstreamSession* session) override;
  void connectError(const folly::AsyncSocketException& ex) override;

  // HTTPTransactionHandler methods
  void setTransaction(proxygen::HTTPTransaction* txn) noexcept override;
  void detachTransaction() noexcept override;

  void onHeadersComplete(
      std::unique_ptr<proxygen::HTTPMessage> msg) noexcept override;
  void onBody(std::unique_ptr<folly::IOBuf> chain) noexcept override;
  void
       onTrailers(std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept override;
  void onEOM() noexcept override;
  void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept override;
  void onError(const proxygen::HTTPException& error) noexcept override;
  void onEgressPaused() noexcept override;
  void onEgressResumed() noexcept override;
  // void onPushedTransaction(
  //     proxygen::HTTPTransaction* /* pushedTxn */) noexcept override;

  // void sendRequest(proxygen::HTTPTransaction* txn);

  // Getters
  std::unique_ptr<folly::IOBuf> getResponseBody();
  // const std::string&            getServerName() const;

  const proxygen::HTTPMessage* getResponse() const {
    return theResponseHeader.get();
  }
  void setEOMFunc(std::function<void()> aEOMFun) {
    theEOMFun = aEOMFun;
  }

 protected:
  // void setupHeaders();

  proxygen::HTTPTransaction* theTxn{nullptr};
  folly::EventBase*          theEvb{nullptr};
  bool                       theEgressPaused{false};
  proxygen::HTTPMethod       theHttpMethod;
  proxygen::URL              theUrl;
  proxygen::HTTPMessage      theRequest;
  int32_t                    theRecvWindow{65536};

  bool           theH2c{false};
  unsigned short theHttpMajor;
  unsigned short theHttpMinor;
  bool           thePartiallyReliable{false};

  std::unique_ptr<proxygen::HTTPMessage> theResponseHeader;
  std::unique_ptr<folly::IOBuf>          theResponseBody;

  folly::Optional<std::function<void()>> theEOMFun;
};

} // namespace edge
} // namespace uiiit