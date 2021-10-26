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

#include "Edge/edgemessages.h"

#include "Edge/Model/chain.h"

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
    , theStates()
    , theCallback()
    , theChain(nullptr)
    , theNextFunctionIndex(0) {
  // noop
}

LambdaRequest::LambdaRequest(const rpc::LambdaRequest& aMsg)
    : theName(aMsg.name())
    , theInput(aMsg.input())
    , theDataIn(aMsg.datain())
    , theForward(true)
    , theHops(aMsg.hops())
    , theStates(deserializeStates(aMsg))
    , theCallback(aMsg.callback())
    , theChain(nullptr)
    , theNextFunctionIndex(aMsg.nextfunctionindex()) {
  if (aMsg.chain_size() > 0) {
    model::Chain::Functions myFunctions;
    for (ssize_t i = 0; i < aMsg.chain_size(); i++) {
      myFunctions.emplace_back(aMsg.chain(i));
    }
    model::Chain::Dependencies myDependencies;
    for (const auto& elem : aMsg.dependencies()) {
      model::Chain::Dependencies::mapped_type myFunctionList;
      for (ssize_t i = 0; i < elem.second.functions_size(); i++) {
        myFunctionList.emplace_back(elem.second.functions(i));
      }
      myDependencies.emplace(elem.first, myFunctionList);
    }
    theChain = std::make_unique<model::Chain>(myFunctions, myDependencies);
  }
}

LambdaRequest::LambdaRequest(LambdaRequest&& aOther)
    : theName(aOther.theName)
    , theInput(aOther.theInput)
    , theDataIn(aOther.theDataIn)
    , theForward(aOther.theForward)
    , theHops(aOther.theHops)
    , theStates(std::move(aOther.theStates))
    , theCallback(std::move(aOther.theCallback))
    , theChain(std::move(aOther.theChain))
    , theNextFunctionIndex(aOther.theNextFunctionIndex) {
  // noop
}

LambdaRequest::~LambdaRequest() {
  // noop
}

rpc::LambdaRequest LambdaRequest::toProtobuf() const {
  rpc::LambdaRequest myRet;
  myRet.set_name(theName);
  myRet.set_input(theInput);
  myRet.set_datain(theDataIn);
  myRet.set_forward(theForward);
  myRet.set_hops(theHops);
  serializeStates(*myRet.mutable_states(), theStates);
  myRet.set_callback(theCallback);
  if (theChain.get() != nullptr) {
    for (const auto& myFunction : theChain->functions()) {
      myRet.add_chain(myFunction);
    }
    for (const auto& elem : theChain->dependencies()) {
      rpc::FunctionList myList;
      for (const auto& myFunction : elem.second) {
        myList.add_functions(myFunction);
      }
      myRet.mutable_dependencies()->insert({elem.first, myList});
    }
  }
  myRet.set_nextfunctionindex(theNextFunctionIndex);
  return myRet;
}

bool LambdaRequest::operator==(const LambdaRequest& aOther) const {
  return theName == aOther.theName and theInput == aOther.theInput and
         theDataIn == aOther.theDataIn /* and theForward == aOther.theForward */
         and theHops == aOther.theHops and theStates == aOther.theStates and
         theCallback == aOther.theCallback and
         ((theChain.get() == nullptr) == (aOther.theChain.get() == nullptr)) and
         (theChain.get() == nullptr or *theChain == *aOther.theChain) and
         theNextFunctionIndex == aOther.theNextFunctionIndex;
}

LambdaRequest LambdaRequest::makeOneMoreHop() const {
  auto ret = copy();
  ret.theHops++;
  return ret;
}

LambdaRequest LambdaRequest::copy() const {
  LambdaRequest ret(theName, theInput, theDataIn, theForward, theHops);
  ret.theStates   = theStates;
  ret.theCallback = theCallback;
  if (theChain.get() != nullptr) {
    ret.theChain = std::make_unique<model::Chain>(*theChain);
  }
  ret.theNextFunctionIndex = theNextFunctionIndex;
  return ret;
}

std::string LambdaRequest::name() const {
  if (theChain.get() == nullptr) {
    return theName;
  }
  return theChain->name();
}

std::string LambdaRequest::toString() const {
  std::stringstream myStream;
  myStream << "name: " << theName << ", "
           << (theForward ? "from edge node" : "from edge client")
           << (theCallback.empty() ? std::string() :
                                     (std::string(", callback ") + theCallback))
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
  if (theChain.get() != nullptr) {
    myStream << ", " << theChain->toString() << ", next function index "
             << theNextFunctionIndex;
    if (theNextFunctionIndex < theChain->functions().size()) {
      myStream << " (" << theChain->functions()[theNextFunctionIndex] << ")";
    }
  }
  return myStream.str();
}

////////////////////////////////////////////////////////////////////////////////
// LambdaResponse
////////////////////////////////////////////////////////////////////////////////

LambdaResponse::LambdaResponse()
    : LambdaResponse("OK", "", {{0.0, 0.0, 0.0}}, true) {
  // noop
}

LambdaResponse::LambdaResponse(const std::string& aRetCode,
                               const std::string& aOutput)
    : LambdaResponse(aRetCode, aOutput, {{0.0, 0.0, 0.0}}, false) {
  // noop
}

LambdaResponse::LambdaResponse(const std::string&           aRetCode,
                               const std::string&           aOutput,
                               const std::array<double, 3>& aLoads)
    : LambdaResponse(aRetCode, aOutput, aLoads, false) {
  // noop
}

LambdaResponse::LambdaResponse(const std::string&           aRetCode,
                               const std::string&           aOutput,
                               const std::array<double, 3>& aLoads,
                               const bool                   aAsynchronous)
    : theRetCode(aRetCode)
    , theOutput(aOutput)
    , theResponder()
    , theProcessingTime(0)
    , theDataOut()
    , theLoad1(0.5 + aLoads[0] * 100)
    , theLoad10(0.5 + aLoads[1] * 100)
    , theLoad30(0.5 + aLoads[2] * 100)
    , theHops(0)
    , theStates()
    , theAsynchronous(aAsynchronous) {
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
    , theStates(deserializeStates(aMsg))
    , theAsynchronous(aMsg.asynchronous()) {
  // noop
}

bool LambdaResponse::operator==(const LambdaResponse& aOther) const {
  return theRetCode == aOther.theRetCode and theOutput == aOther.theOutput and
         theResponder == aOther.theResponder and
         theProcessingTime == aOther.theProcessingTime and
         theDataOut == aOther.theDataOut and theLoad1 == aOther.theLoad1 and
         theLoad10 == aOther.theLoad10 and theLoad30 == aOther.theLoad30 and
         theHops == aOther.theHops and theStates == aOther.theStates and
         theAsynchronous == aOther.theAsynchronous;
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
  myRet.set_asynchronous(theAsynchronous);
  return myRet;
}

std::string LambdaResponse::toString() const {
  std::stringstream myStream;
  myStream << "retcode: " << theRetCode;
  if (theAsynchronous) {
    myStream << ", asynchronous";
  } else {
    myStream << ", from: " << theResponder << ", ptime: " << theProcessingTime
             << " ms"
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
