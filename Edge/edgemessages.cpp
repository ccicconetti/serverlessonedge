/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2021 C. Cicconetti <https://ccicconetti.github.io/>

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
// State
////////////////////////////////////////////////////////////////////////////////

State::State(const rpc::State& aState)
    : theContent(aState.content()) {
  // noop
}

rpc::State State::toProtobuf() const {
  rpc::State ret;
  ret.set_content(theContent);
  return ret;
}

bool State::operator==(const State& aOther) const {
  return theContent == aOther.theContent;
}

////////////////////////////////////////////////////////////////////////////////
// LambdaRequest
////////////////////////////////////////////////////////////////////////////////

LambdaRequest::LambdaRequest(const std::string& aName,
                             const std::string& aInput)
    : LambdaRequest(aName, aInput, std::string(), false, 0) {
  // noop
}

LambdaRequest::LambdaRequest(const std::string& aName,
                             const std::string& aInput,
                             const std::string& aDataIn)
    : LambdaRequest(aName, aInput, aDataIn, false, 0) {
  // noop
}

LambdaRequest::LambdaRequest(const rpc::LambdaRequest& aMsg)
    : theName(aMsg.name())
    , theInput(aMsg.input())
    , theDataIn(aMsg.datain())
    , theForward(true)
    , theHops(aMsg.hops())
    , theStates(deserializeStates(aMsg)) {
  // noop
}

bool LambdaRequest::operator==(const LambdaRequest& aOther) const {
  return theName == aOther.theName and theInput == aOther.theInput and
         theDataIn == aOther.theDataIn /* and theForward == aOther.theForward */
         and theHops == aOther.theHops and theStates == aOther.theStates;
}

rpc::LambdaRequest LambdaRequest::toProtobuf() const {
  rpc::LambdaRequest myRet;
  myRet.set_name(theName);
  myRet.set_input(theInput);
  myRet.set_datain(theDataIn);
  myRet.set_forward(theForward);
  myRet.set_hops(theHops);
  serializeStates(*myRet.mutable_states(), theStates);
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
    , theHops(aHops)
    , theStates() {
}

LambdaRequest LambdaRequest::makeOneMoreHop() const {
  return LambdaRequest(theName, theInput, theDataIn, theForward, theHops + 1);
}

std::string LambdaRequest::toString() const {
  std::stringstream myStream;
  myStream << "name: " << theName << ", "
           << (theForward ? "from edge node" : "from edge client")
           << ", hops: " << theHops << ", input: " << theInput
           << ", datain size: " << theDataIn.size();
  if (not theStates.empty()) {
    myStream << ", states: [";
    for (auto it = theStates.cbegin(); it != theStates.end(); ++it) {
      if (it != theStates.cbegin()) {
        myStream << ", ";
      }
      myStream << it->first << " (" << it->second.theContent.size()
               << " bytes)";
    }
    myStream << "]";
  }
  return myStream.str();
}

////////////////////////////////////////////////////////////////////////////////
// LambdaResponse
////////////////////////////////////////////////////////////////////////////////

LambdaResponse::LambdaResponse(const std::string& aRetCode,
                               const std::string& aOutput)
    : LambdaResponse(aRetCode, aOutput, {{0.0, 0.0, 0.0}}) {
  // noop
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
    , theLoad30(0.5 + aLoads[2] * 100)
    , theHops(0)
    , theStates() {
  // noop
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
    , theHops(aMsg.hops())
    , theStates(deserializeStates(aMsg)) {
  // noop
}

bool LambdaResponse::operator==(const LambdaResponse& aOther) const {
  return theRetCode == aOther.theRetCode and theOutput == aOther.theOutput and
         theResponder == aOther.theResponder and
         theProcessingTime == aOther.theProcessingTime and
         theDataOut == aOther.theDataOut and theLoad1 == aOther.theLoad1 and
         theLoad10 == aOther.theLoad10 and theLoad30 == aOther.theLoad30 and
         theHops == aOther.theHops and theStates == aOther.theStates;
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
  serializeStates(*myRet.mutable_states(), theStates);
  return myRet;
}

std::string LambdaResponse::toString() const {
  std::stringstream myStream;
  myStream << "retcode: " << theRetCode << ", from: " << theResponder
           << ", ptime: " << theProcessingTime << " ms"
           << ", hops: " << theHops << ", load: " << theLoad1 << "/"
           << theLoad10 << "/" << theLoad30 << ", output: " << theOutput
           << ", dataout size: " << theDataOut.size();
  if (not theStates.empty()) {
    myStream << ", states: [";
    for (auto it = theStates.cbegin(); it != theStates.end(); ++it) {
      if (it != theStates.cbegin()) {
        myStream << ", ";
      }
      myStream << it->first << " (" << it->second.theContent.size()
               << " bytes)";
    }
    myStream << "]";
  }
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
