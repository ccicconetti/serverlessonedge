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

#include <proxygen/lib/http/session/HTTPUpstreamSession.h>

namespace uiiit {
namespace edge {

CurlClient::CurlClient(folly::EventBase*            aEvb,
                       proxygen::HTTPMethod         aHttpMethod,
                       const proxygen::URL&         aUrl,
                       const proxygen::HTTPHeaders& aHeaders,
                       bool                         aH2c,
                       unsigned short               aHttpMajor,
                       unsigned short               aHttpMinor,
                       bool                         aPartiallyReliable) {
  VLOG(1) << "CurlClient::ctor";
}

// proxygen::HTTPHeaders
// CurlClient::parseHeaders(const std::string& headersString) {
//   std::vector<folly::StringPiece> headersList;
//   proxygen::HTTPHeaders           headers;
//   folly::split(",", headersString, headersList);
//   for (const auto& headerPair : headersList) {
//     std::vector<folly::StringPiece> nv;
//     folly::split('=', headerPair, nv);
//     if (nv.size() > 0) {
//       if (nv[0].empty()) {
//         continue;
//       }
//       std::string value("");
//       for (size_t i = 1; i < nv.size(); i++) {
//         value += folly::to<std::string>(nv[i], '=');
//       }
//       if (nv.size() > 1) {
//         value.pop_back();
//       } // trim anything else
//       headers.add(nv[0], value);
//     }
//   }
//   return headers;
// }

void CurlClient::connectSuccess(proxygen::HTTPUpstreamSession* session) {
  LOG(INFO) << "CurlClient::ConnectSuccess";
  session->setFlowControl(theRecvWindow, theRecvWindow, theRecvWindow);
  // sendRequest(session->newTransaction(this));
  // session->closeWhenIdle();
}

/*
void CurlClient::sendRequest(HTTPTransaction* txn) {
  txn_ = txn;
  setupHeaders();
  txn_->sendHeaders(request_);

  if (httpMethod_ == HTTPMethod::POST) {
    inputFile_ =
        std::make_unique<ifstream>(inputFilename_, ios::in | ios::binary);
    while (inputFile_->good() && !egressPaused_) {
    unique_ptr<IOBuf> buf = IOBuf::createCombined(kReadSize);
    inputFile_->read((char*)buf->writableData(), kReadSize);
    buf->append(inputFile_->gcount());
    txn_->sendBody(move(buf));
    }
    if (!egressPaused_) {
      txn_->sendEOM();
    }
  } else {
    txn_->sendEOM();
  }
}
*/

void CurlClient::connectError(const folly::AsyncSocketException& aEx) {
  LOG(ERROR) << "Coudln't connect to " << theUrl.getHostAndPort() << ":"
             << aEx.what();
}

void CurlClient::setTransaction(proxygen::HTTPTransaction*) noexcept {
}

void CurlClient::detachTransaction() noexcept {
}

void CurlClient::onHeadersComplete(
    std::unique_ptr<proxygen::HTTPMessage> msg) noexcept {
  LOG(INFO) << "CurlClient::onHeadersComplete";
  theResponseHeader = std::move(msg);
}

void CurlClient::onBody(std::unique_ptr<folly::IOBuf> aChain) noexcept {
  VLOG(1) << "CurlClient::onBody()";
  if (aChain)
    theResponseBody = std::move(aChain);
}

void CurlClient::onTrailers(std::unique_ptr<proxygen::HTTPHeaders>) noexcept {
  LOG(INFO) << "Discarding trailers";
}

void CurlClient::onEOM() noexcept {
  LOG(INFO) << "CurlClient::onEOM";
  if (theEOMFun) {
    theEOMFun.value()();
  }
}

void CurlClient::onUpgrade(proxygen::UpgradeProtocol) noexcept {
  LOG(INFO) << "Discarding upgrade protocol";
}

void CurlClient::onEgressPaused() noexcept {
  LOG(INFO) << "Egress paused";
  theEgressPaused = true;
}

void CurlClient::onEgressResumed() noexcept {
  LOG(INFO) << "Egress resumed";
  theEgressPaused = false;
}

void CurlClient::onError(const proxygen::HTTPException& aError) noexcept {
  LOG(ERROR) << "An error occurred: " << aError.what();
}

std::unique_ptr<folly::IOBuf> CurlClient::getResponseBody() {
  VLOG(1) << "CurlClient::getResponseBody()";
  return std::move(theResponseBody);
}

} // namespace edge
} // namespace uiiit