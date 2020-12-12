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

#include "Quic/basehandler.h"

namespace uiiit {
namespace edge {

class EchoHandler : public BaseHandler
{
 public:
  explicit EchoHandler(const HQParams& params)
      : BaseHandler(params) {
  }

  EchoHandler() = delete;

  void onHeadersComplete(
      std::unique_ptr<proxygen::HTTPMessage> msg) noexcept override {
    VLOG(10) << "EchoHandler::onHeadersComplete";
    proxygen::HTTPMessage resp;
    VLOG(10) << "Setting http-version to " << getHttpVersion();
    sendFooter_ =
        (msg->getHTTPVersion() == proxygen::HTTPMessage::kHTTPVersion09);
    resp.setVersionString(getHttpVersion());
    resp.setStatusCode(200);
    resp.setStatusMessage("Ok");
    msg->getHeaders().forEach(
        [&](const std::string& header, const std::string& val) {
          resp.getHeaders().add(folly::to<std::string>("x-echo-", header), val);
        });
    resp.setWantsKeepalive(true);
    maybeAddAltSvcHeader(resp);
    txn_->sendHeaders(resp);
  }

  void onBody(std::unique_ptr<folly::IOBuf> chain) noexcept override {
    VLOG(10) << "EchoHandler::onBody";
    txn_->sendBody(std::move(chain));
  }

  void onEOM() noexcept override {
    VLOG(10) << "EchoHandler::onEOM";
    if (sendFooter_) {
      auto& footer = getH1QFooter();
      txn_->sendBody(folly::IOBuf::copyBuffer(footer.data(), footer.length()));
    }
    txn_->sendEOM();
  }

  void onError(const proxygen::HTTPException& /*error*/) noexcept override {
    txn_->sendAbort();
  }

 private:
  bool sendFooter_{false};
};

} // namespace edge
} // namespace uiiit