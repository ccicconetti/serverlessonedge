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
  explicit LambdaRequestHandler(const HQParams&    aQuicParamsConf,
                                EdgeServerQuic&    aEdgeServer,
                                const std::string& aServerEndpoint)
      : BaseHandler(aQuicParamsConf)
      , theEdgeServer(aEdgeServer)
      , theResponder(aServerEndpoint) {
    VLOG(4) << "LambdaRequestHandler::ctor\n";
  }

  LambdaRequestHandler() = delete;

  void onHeadersComplete(
      std::unique_ptr<proxygen::HTTPMessage> aMsg) noexcept override {
    VLOG(4) << "LambdaRequestHandler::onHeadersComplete";
    VLOG(4) << "Setting http-version to " << getHttpVersion();

    theResponse.setVersionString(getHttpVersion());
    theResponse.setWantsKeepalive(true);
    maybeAddAltSvcHeader(theResponse);
  }

  void onBody(std::unique_ptr<folly::IOBuf> aChain) noexcept override {
    VLOG(4) << "LambdaRequestHandler::onBody";

    if (theRequestBody) {
      theRequestBody->prependChain(std::move(aChain));
    } else {
      theRequestBody = std::move(aChain);
    }
  }

  void onEOM() noexcept override {
    VLOG(4) << "LambdaRequestHandler::onEOM";

    // converting the folly::IOBuf Chain in a rpc::LambdaRequest
    auto coalescedIOBuf = theRequestBody->coalesce();

    rpc::LambdaRequest myProtobufLambdaReq;
    myProtobufLambdaReq.ParseFromArray(coalescedIOBuf.data(),
                                       coalescedIOBuf.size());

    // useful for debugging
    // if (VLOG_IS_ON(4)) {
    //   LambdaRequest myLambdaReq(myProtobufLambdaReq);
    //   LOG(INFO) << "LambdaRequest Received = " << myLambdaReq.toString();
    // }

    // actual LambdaRequest processing
    rpc::LambdaResponse myProtobufLambdaResp =
        theEdgeServer.process(myProtobufLambdaReq);

    // building of the LambdaResponse
    LambdaResponse myLambdaResp(myProtobufLambdaResp);
    // LOG(INFO) << "LambdaResponse Produced = " << myLambdaResp.toString();

    if (myLambdaResp.theRetCode == std::string("OK")) {
      theResponse.setStatusCode(200);
      theResponse.setStatusMessage("Ok");
    } else {
      theResponse.setStatusCode(400);
      theResponse.setStatusMessage("Bad Request");
    }
    theTransaction->sendHeaders(theResponse);

    if (myLambdaResp.theResponder.empty()) {
      VLOG(4) << "theResponder = " << myLambdaResp.theResponder;
      myProtobufLambdaResp.set_responder(theResponder);
    }

    // send the LambdaResponse as body of the HTTPResponse
    size_t size   = myProtobufLambdaResp.ByteSizeLong();
    void*  buffer = malloc(size);
    myProtobufLambdaResp.SerializeToArray(buffer, size);
    auto buf = folly::IOBuf::copyBuffer(buffer, size);
    theTransaction->sendBody(std::move(buf));

    theTransaction->sendEOM();
  }

  void onError(const proxygen::HTTPException& /*error*/) noexcept override {
    LOG(ERROR) << "LambdaRequestHandler::onError";
    theTransaction->sendAbort();
  }

 private:
  EdgeServerQuic&               theEdgeServer;
  proxygen::HTTPMessage         theResponse;
  const std::string             theResponder;
  std::unique_ptr<folly::IOBuf> theRequestBody;
};

} // namespace edge
} // namespace uiiit