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
                       const rpc::LambdaRequest     aLambdaRequest,
                       bool                         aH2c,
                       unsigned short               aHttpMajor,
                       unsigned short               aHttpMinor)
    : theEvb(aEvb)
    , theHttpMethod(aHttpMethod)
    , theUrl(aUrl)
    , theLambdaRequest(aLambdaRequest)
    , theH2c(aH2c)
    , theHttpMajor(aHttpMajor)
    , theHttpMinor(aHttpMinor) {
  VLOG(1) << "CurlClient::ctor";
}

void CurlClient::connectSuccess(proxygen::HTTPUpstreamSession* aSession) {
  LOG(INFO) << "CurlClient::ConnectSuccess";

  aSession->setFlowControl(theRecvWindow, theRecvWindow, theRecvWindow);
  // sendRequest(aSession->newTransaction(this));
  // aSession->closeWhenIdle();
}

void CurlClient::setupHeaders() {
  theRequestHTTPMessage.setMethod(theHttpMethod);
  theRequestHTTPMessage.setHTTPVersion(1, 1);

  theRequestHTTPMessage.setURL(theUrl.makeRelativeURL());
  theRequestHTTPMessage.setSecure(theUrl.isSecure());

  if (!theRequestHTTPMessage.getHeaders().getNumberOfValues(
          proxygen::HTTP_HEADER_USER_AGENT)) {
    theRequestHTTPMessage.getHeaders().add(proxygen::HTTP_HEADER_USER_AGENT,
                                           "proxygen_curl");
  }
  if (!theRequestHTTPMessage.getHeaders().getNumberOfValues(
          proxygen::HTTP_HEADER_HOST)) {
    theRequestHTTPMessage.getHeaders().add(proxygen::HTTP_HEADER_HOST,
                                           theUrl.getHostAndPort());
  }
  if (!theRequestHTTPMessage.getHeaders().getNumberOfValues(
          proxygen::HTTP_HEADER_ACCEPT)) {
    theRequestHTTPMessage.getHeaders().add("Accept", "*/*");
  }
}

void CurlClient::sendRequest(proxygen::HTTPTransaction* aTxn) {
  LOG(INFO) << "CurlClient::sendRequest";
  theTxn = aTxn;
  setupHeaders();
  theRequestHTTPMessage.setIsChunked(true);
  theTxn->sendHeaders(theRequestHTTPMessage);

  size_t mySize   = theLambdaRequest.ByteSizeLong();
  void*  myBuffer = malloc(mySize);
  theLambdaRequest.SerializeToArray(myBuffer, mySize);
  auto myIOBuf = folly::IOBuf::copyBuffer(myBuffer, mySize);
  theTxn->sendBody(std::move(myIOBuf));

  if (!theEgressPaused) {
    theTxn->sendEOM();
  }
} // namespace edge

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
  theEvb->terminateLoopSoon();
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