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

#pragma once

#include "edgeserver.grpc.pb.h"

#include <array>
#include <iostream>
#include <memory>
#include <string>

namespace uiiit {
namespace edge {

namespace model {
class Chain;
};

//! An application's state.
struct State {
  //! Create with given content.
  explicit State(const std::string& aContent)
      : theContent(aContent) {
    // noop
  }

  //! Create from protobuf.
  explicit State(const rpc::State& aState);

  //! \return a serialized protobuf message.
  rpc::State toProtobuf() const;

  //! \return true if the states are identical.
  bool operator==(const State& aOther) const;

  //! The content of this state.
  std::string theContent;
};

//! A function request, with arguments and possibly also embeddeding states.
struct LambdaRequest final {
  /**
   * Create a lambda request with text input only and no states.
   *
   * \param aName lambda function name.
   * \param aInput function input (text).
   *
   * The forward flag is not set.
   */
  explicit LambdaRequest(const std::string& aName, const std::string& aInput);
  /**
   * Create a lambda request with text and data input only and no states.
   *
   * \param aName lambda function name.
   * \param aInput function input (text); can be empty.
   * \param aDataIn function input (data); can be empty.
   *
   * The forward flag is not set.
   */
  explicit LambdaRequest(const std::string& aName,
                         const std::string& aInput,
                         const std::string& aDataIn);
  /**
   * Create a lambda request from an underlying protobuf.
   *
   * The forward flag is automatically set.
   */
  explicit LambdaRequest(const rpc::LambdaRequest& aMsg);

  LambdaRequest(LambdaRequest&& aOther);

  LambdaRequest(LambdaRequest&) = delete;

  ~LambdaRequest();

  //! \return true if the messages are identical except for the forward flag.
  bool operator==(const LambdaRequest& aOther) const;

  //! \return a lambda request identical to the input with +1 hops.
  LambdaRequest makeOneMoreHop() const;

  //! \return a lambda request identical to this one.
  LambdaRequest copy() const;

  /**
   * @brief Return the request name
   *
   * @return lambda if the is no function chain, otherwise a mangle of the
   * functions to be invoked.
   */
  std::string name() const;

  //! \return the application' states.
  std::map<std::string, State>& states() {
    return theStates;
  }

  //! \return the application' states.
  const std::map<std::string, State>& states() const {
    return theStates;
  }

  //! \return the protobuf-encoded message.
  rpc::LambdaRequest toProtobuf() const;
  //! \return a human-readable representation of the request.
  std::string toString() const;

  const std::string             theName;
  const std::string             theInput;
  const std::string             theDataIn;
  const bool                    theForward;
  unsigned int                  theHops;
  std::map<std::string, State>  theStates;
  std::string                   theCallback;
  std::unique_ptr<model::Chain> theChain;
  unsigned int                  theNextFunctionIndex;

 private:
  explicit LambdaRequest(const std::string& aName,
                         const std::string& aInput,
                         const std::string& aDataIn,
                         const bool         aForward,
                         const unsigned int aHops);
};

//! A function return, possibly also embeddeding states.
struct LambdaResponse final {
  //! Create an asynchronous response (without output).
  explicit LambdaResponse();

  //! Create a synchronous response (with output).
  explicit LambdaResponse(const std::string& aRetCode,
                          const std::string& aOutput);

  //! Create a synchronous response (with output), with load indications.
  explicit LambdaResponse(const std::string&           aRetCode,
                          const std::string&           aOutput,
                          const std::array<double, 3>& aLoads);

  //! Deserialize from protobuf.
  explicit LambdaResponse(const rpc::LambdaResponse& aMsg);

  //! \return true if the messages are identical.
  bool operator==(const LambdaResponse& aOther) const;

  //! \return the processing time, in fractional seconds.
  double processingTimeSeconds() const noexcept;

  //! \return the application' states.
  std::map<std::string, State>& states() {
    return theStates;
  }

  //! \return the application' states.
  const std::map<std::string, State>& states() const {
    return theStates;
  }

  //! Remove the processing time and load info.
  void removePtimeLoad();

  //! \return the protobuf-encoded message.
  rpc::LambdaResponse toProtobuf() const;
  //! \return a human-readable representation of the request.
  std::string toString() const;

  const std::string            theRetCode;
  const std::string            theOutput;
  std::string                  theResponder;
  unsigned int                 theProcessingTime;
  std::string                  theDataOut;
  unsigned short               theLoad1;
  unsigned short               theLoad10;
  unsigned short               theLoad30;
  unsigned int                 theHops;
  std::map<std::string, State> theStates;
  const bool                   theAsynchronous;

 private:
  explicit LambdaResponse(const std::string&           aRetCode,
                          const std::string&           aOutput,
                          const std::array<double, 3>& aLoads,
                          const bool                   aAsynchronous);
};

// free functions
template <class Message>
std::map<std::string, State> deserializeStates(const Message& aMessage) {
  std::map<std::string, State> ret;
  for (const auto& elem : aMessage.states()) {
    ret.emplace(elem.first, State(elem.second));
  }
  return ret;
}

template <class Map>
void serializeStates(Map& aMap, const std::map<std::string, State>& aStates) {
  for (const auto& elem : aStates) {
    aMap.insert({elem.first, elem.second.toProtobuf()});
  }
}

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                     aStream,
                         const uiiit::edge::LambdaRequest& aReq);
std::ostream& operator<<(std::ostream&                      aStream,
                         const uiiit::edge::LambdaResponse& aRep);
