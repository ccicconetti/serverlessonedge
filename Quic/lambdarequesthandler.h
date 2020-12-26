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
  explicit LambdaRequestHandler(const HQParams& params, EdgeServer& aEdgeServer)
      : BaseHandler(params)
      , theEdgeServer(aEdgeServer) {
    VLOG(10) << "LambdaRequestHandler::CTOR\n";
  }

  LambdaRequestHandler() = delete;

  void onHeadersComplete(
      std::unique_ptr<proxygen::HTTPMessage> msg) noexcept override {
    VLOG(10) << "LambdaRequestHandler::onHeadersComplete";
    VLOG(10) << "Setting http-version to " << getHttpVersion();

    theResponse.setVersionString(getHttpVersion());
    theResponse.setWantsKeepalive(true);
    maybeAddAltSvcHeader(theResponse);
  }

  void onBody(std::unique_ptr<folly::IOBuf> chain) noexcept override {
    VLOG(10) << "LambdaRequestHandler::onBody";

    // converting the folly::IOBuf in a rpc::LambdaRequest
    rpc::LambdaRequest theProtobufLambdaReq;
    theProtobufLambdaReq.ParseFromArray(chain->data(), chain->length());

    // useful for debugging
    LambdaRequest theLambdaReq(theProtobufLambdaReq);
    VLOG(10) << "LambdaRequest.toString() = \n" << theLambdaReq.toString();

    // actual LambdaRequest processing
    rpc::LambdaResponse aLambdaRes =
        theEdgeServer.process(theProtobufLambdaReq);

    // building of the LambdaResponse in order to check for theRetCode and set
    // the HTTPMessage theStatusCode and theStatusMessage according to it
    LambdaResponse theLambdaRes(aLambdaRes);
    if (theLambdaRes.theRetCode == std::string("OK")) {
      theResponse.setStatusCode(200);
      theResponse.setStatusMessage("Ok");
    } else {
      theResponse.setStatusCode(400);
      theResponse.setStatusMessage("Bad Request");
    }
    txn_->sendHeaders(theResponse);

    // send the LambdaResponse as body of the HTTPResponse
    size_t size   = aLambdaRes.ByteSizeLong();
    void*  buffer = malloc(size);
    aLambdaRes.SerializeToArray(buffer, size);
    auto buf = folly::IOBuf::copyBuffer(buffer, size);
    txn_->sendBody(std::move(buf));
  }

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
  EdgeServer&           theEdgeServer;
  proxygen::HTTPMessage theResponse;
};

} // namespace edge
} // namespace uiiit