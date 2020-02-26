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

#include "edgemessages.h"

#include <sstream>

namespace uiiit {
namespace edge {

////////////////////////////////////////////////////////////////////////////////
// LambdaRequest
////////////////////////////////////////////////////////////////////////////////

LambdaRequest::LambdaRequest(const std::string& aName,
                             const std::string& aInput)
    : LambdaRequest(aName, aInput, std::string(), false, 0) {
}

LambdaRequest::LambdaRequest(const std::string& aName,
                             const std::string& aInput,
                             const std::string& aDataIn)
    : LambdaRequest(aName, aInput, aDataIn, false, 0) {
}

LambdaRequest::LambdaRequest(const rpc::LambdaRequest& aMsg)
    : theName(aMsg.name())
    , theInput(aMsg.input())
    , theDataIn(aMsg.datain())
    , theForward(true)
    , theHops(aMsg.hops()) {
}

rpc::LambdaRequest LambdaRequest::toProtobuf() const {
  rpc::LambdaRequest myRet;
  myRet.set_name(theName);
  myRet.set_input(theInput);
  myRet.set_datain(theDataIn);
  myRet.set_forward(theForward);
  myRet.set_hops(theHops);
  return myRet;
}

LambdaRequest::LambdaRequest(const std::string& aName,
                             const std::string& aInput,
                             const std::string& aDataIn,
                             const bool         aForward,
                             const unsigned int aHops)
    : theName(aName)
    , theInput(aInput)
    , theDataIn(aDataIn)
    , theForward(aForward)
    , theHops(aHops) {
}

LambdaRequest LambdaRequest::makeOneMoreHop() const {
  return LambdaRequest(theName,
                       theInput,
                       theDataIn,
                       theForward,
                       theHops + 1);
}

std::string LambdaRequest::toString() const {
  std::stringstream myStream;
  myStream << "name: " << theName << ", "
           << (theForward ? "from edge node" : "from edge client")
           << ", hops: " << theHops << ", input: " << theInput
           << ", datain size: " << theDataIn.size();
  return myStream.str();
}

////////////////////////////////////////////////////////////////////////////////
// LambdaResponse
////////////////////////////////////////////////////////////////////////////////

LambdaResponse::LambdaResponse(const std::string& aRetCode,
                               const std::string& aOutput)
    : LambdaResponse(aRetCode, aOutput, {{0.0, 0.0, 0.0}}) {
}

LambdaResponse::LambdaResponse(const std::string&           aRetCode,
                               const std::string&           aOutput,
                               const std::array<double, 3>& aLoads)
    : theRetCode(aRetCode)
    , theOutput(aOutput)
    , theResponder()
    , theProcessingTime(0)
    , theDataOut()
    , theLoad1(0.5 + aLoads[0] * 100)
    , theLoad10(0.5 + aLoads[1] * 100)
    , theLoad30(0.5 + aLoads[2] * 100),theHops(0) {
}

LambdaResponse::LambdaResponse(const rpc::LambdaResponse& aMsg)
    : theRetCode(aMsg.retcode())
    , theOutput(aMsg.output())
    , theResponder(aMsg.responder())
    , theProcessingTime(aMsg.ptime())
    , theDataOut(aMsg.dataout())
    , theLoad1(aMsg.load1())
    , theLoad10(aMsg.load10())
    , theLoad30(aMsg.load30())
    , theHops(aMsg.hops()) {
}

rpc::LambdaResponse LambdaResponse::toProtobuf() const {
  rpc::LambdaResponse myRet;
  myRet.set_retcode(theRetCode);
  myRet.set_output(theOutput);
  myRet.set_responder(theResponder);
  myRet.set_ptime(theProcessingTime);
  myRet.set_dataout(theDataOut);
  myRet.set_load1(theLoad1);
  myRet.set_load10(theLoad10);
  myRet.set_load30(theLoad30);
  myRet.set_hops(theHops);
  return myRet;
}

std::string LambdaResponse::toString() const {
  std::stringstream myStream;
  myStream << "retcode: " << theRetCode << ", from: " << theResponder
           << ", ptime: " << theProcessingTime << " ms"
           << ", hops: " << theHops << ", load: " << theLoad1 << "/"
           << theLoad10 << "/" << theLoad30 << ", output: " << theOutput
           << ", dataout size: " << theDataOut.size();
  return myStream.str();
}

double LambdaResponse::processingTimeSeconds() const noexcept {
  return theProcessingTime * 1e-3;
}

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                     aStream,
                         const uiiit::edge::LambdaRequest& aReq) {
  aStream << aReq.toString();
  return aStream;
}

std::ostream& operator<<(std::ostream&                      aStream,
                         const uiiit::edge::LambdaResponse& aRep) {
  aStream << aRep.toString();
  return aStream;
}
