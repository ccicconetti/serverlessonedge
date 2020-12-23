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

#include "Edge/edgemessages.h"
#include "Quic/basehandler.h"

namespace uiiit {
namespace edge {

class LambdaRequestHandler : public BaseHandler
{
 public:
  explicit LambdaRequestHandler(
      const HQParams& params) //, EdgeServer& aEdgeServer)
      : BaseHandler(params) {
    //, theEdgeServer(aEdgeServer) {
    LOG(INFO) << "LambdaRequestHandler::CTOR\n";
  }

  LambdaRequestHandler() = delete;

  void onHeadersComplete(
      std::unique_ptr<proxygen::HTTPMessage> msg) noexcept override {
    VLOG(10) << "LambdaRequestHandler::onHeadersComplete";

    // HTTPResponse building, put as member if the response must be sent after
    // LambdaRequest processing or after EOM
    proxygen::HTTPMessage resp;
    VLOG(10) << "Setting http-version to " << getHttpVersion();

    // move sendHeaders after LambdaRequest processing
    resp.setVersionString(getHttpVersion());
    resp.setStatusCode(200);
    resp.setStatusMessage("Ok");
    msg->getHeaders().forEach([&](const std::string& header,
                                  const std::string& val) {
      resp.getHeaders().add(folly::to<std::string>("lambda-execution-", header),
                            val);
    });
    resp.setWantsKeepalive(true);
    maybeAddAltSvcHeader(resp);
    txn_->sendHeaders(resp);
  }

  void onBody(std::unique_ptr<folly::IOBuf> chain) noexcept override {
    VLOG(10) << "LambdaRequestHandler::onBody";

    // LOG(INFO) << chain->headroom();
    // LOG(INFO) << chain->tailroom();
    // LOG(INFO) << chain->length();
    // LOG(INFO) << chain->capacity();

    // converting the folly::IOBuf to a LambdaRequest
    rpc::LambdaRequest theProtobufLambdaReq;
    theProtobufLambdaReq.ParseFromArray(chain->data(), chain->length());
    LambdaRequest theLambdaReq(theProtobufLambdaReq);
    LOG(INFO) << "LambdaRequest.toString() = \n" << theLambdaReq.toString();

    /**
     * invoke theEdgeServer.process (need to find a way to pass theEdgeServer
     * reference)
     */
    // LambdaResponse lambdaRes = theEdgeServer.process(lambdaReq);

    /**
     * need to construct a new IOBuf in order to wrap the response and
     * then to send it to the client in the body of the HTTPRes
     */
    txn_->sendBody(std::move(chain));
  }

  // need to see if the HTTPResponse should be sent in onEOM body or in onBody
  // body
  void onEOM() noexcept override {
    VLOG(10) << "LambdaRequestHandler::onEOM";
    txn_->sendEOM();
  }

  // when is this callback called? does it need to send an HTTPResponse with
  // error status?
  void onError(const proxygen::HTTPException& /*error*/) noexcept override {
    txn_->sendAbort();
  }

 private:
  // EdgeServer&        theEdgeServer;
};

} // namespace edge
} // namespace uiiit