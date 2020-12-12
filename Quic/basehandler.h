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

#include "Quic/quicparams.h"

#include <folly/Format.h>
//#include <proxygen/httpserver/samples/hq/HQParams.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>

namespace uiiit {
namespace edge {

class BaseHandler : public proxygen::HTTPTransactionHandler
{
 public:
  BaseHandler() = delete;

  // explicit BaseHandler(const quic::samples::HQParams& params)
  explicit BaseHandler(const HQParams& params)
      : params_(params) {
  }

  void setTransaction(proxygen::HTTPTransaction* txn) noexcept override {
    txn_ = txn;
  }

  void detachTransaction() noexcept override {
    delete this;
  }

  void onChunkHeader(size_t /*length*/) noexcept override {
  }

  void onChunkComplete() noexcept override {
  }

  void onTrailers(
      std::unique_ptr<proxygen::HTTPHeaders> /*trailers*/) noexcept override {
  }

  void onUpgrade(proxygen::UpgradeProtocol /*protocol*/) noexcept override {
  }

  void onEgressPaused() noexcept override {
  }

  void onEgressResumed() noexcept override {
  }

  void maybeAddAltSvcHeader(proxygen::HTTPMessage& msg) const {
    if (params_.protocol.empty() || params_.port == 0) {
      return;
    }
    msg.getHeaders().add(
        proxygen::HTTP_HEADER_ALT_SVC,
        folly::format("{}=\":{}\"; ma=3600", params_.protocol, params_.port)
            .str());
  }

  // clang-format off
  static const std::string& getH1QFooter() {
    static const std::string footer(
" __    __  .___________.___________..______      ___ ___       ___    ______\n"
"|  |  |  | |           |           ||   _  \\    /  // _ \\     / _ \\  |      \\\n"
"|  |__|  | `---|  |----`---|  |----`|  |_)  |  /  /| | | |   | (_) | `----)  |\n"
"|   __   |     |  |        |  |     |   ___/  /  / | | | |    \\__, |     /  /\n"
"|  |  |  |     |  |        |  |     |  |     /  /  | |_| |  __  / /     |__|\n"
"|__|  |__|     |__|        |__|     | _|    /__/    \\___/  (__)/_/       __\n"
"                                                                        (__)\n"
"\n"
"\n"
"____    __    ____  __    __       ___   .___________.\n"
"\\   \\  /  \\  /   / |  |  |  |     /   \\  |           |\n"
" \\   \\/    \\/   /  |  |__|  |    /  ^  \\ `---|  |----`\n"
"  \\            /   |   __   |   /  /_\\  \\    |  |\n"
"   \\    /\\    /    |  |  |  |  /  _____  \\   |  |\n"
"    \\__/  \\__/     |__|  |__| /__/     \\__\\  |__|\n"
"\n"
" ____    ____  _______     ___      .______\n"
"\\   \\  /  / |   ____|   /   \\     |   _  \\\n"
" \\   \\/  /  |  |__     /  ^  \\    |  |_)  |\n"
"  \\_    _/   |   __|   /  /_\\ \\   |      /\n"
"     |  |     |  |____ /  _____  \\  |  |\\  \\----.\n"
"     |__|     |_______/__/     \\__\\| _| `._____|\n"
"\n"
" __       _______.    __  .___________.______\n"
"|  |     /       |   |  | |           |      \\\n"
"|  |    |   (----`   |  | `---|  |----`----)  |\n"
"|  |     \\   \\     |  |     |  |        /  /\n"
"|  | .----)   |      |  |     |  |       |__|\n"
"|__| |_______/       |__|     |__|        __\n"
"                                         (__)\n"
    );
    // clang-format on
    return footer;
  }

 protected:
  const std::string& getHttpVersion() const {
    return params_.httpVersion.canonical;
  }

  proxygen::HTTPTransaction* txn_{nullptr};
  // const quic::samples::HQParams& params_;
  const HQParams& params_;
};

} // namespace edge
} // namespace uiiit