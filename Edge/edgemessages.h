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
#include <string>

namespace uiiit {
namespace edge {

struct LambdaRequest {
  /**
   * Create a lambda request with text input only.
   *
   * \param aName lambda function name.
   * \param aInput function input (text).
   *
   * The forward flag is not set.
   */
  explicit LambdaRequest(const std::string& aName, const std::string& aInput);
  /**
   * Create a lambda request with text and data input only.
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

  //! Create a lambda request identical to the input with +1 hops.
  LambdaRequest makeOneMoreHop() const;

  rpc::LambdaRequest toProtobuf() const;
  std::string        toString() const;

  const std::string  theName;
  const std::string  theInput;
  const std::string  theDataIn;
  const bool         theForward;
  const unsigned int theHops;

 private:
  explicit LambdaRequest(const std::string& aName,
                         const std::string& aInput,
                         const std::string& aDataIn,
                         const bool         aForward,
                         const unsigned int aHops);
};

struct LambdaResponse {
  explicit LambdaResponse(const std::string& aRetCode,
                          const std::string& aOutput);
  explicit LambdaResponse(const std::string&           aRetCode,
                          const std::string&           aOutput,
                          const std::array<double, 3>& aLoads);
  explicit LambdaResponse(const rpc::LambdaResponse& aMsg);

  rpc::LambdaResponse toProtobuf() const;
  std::string         toString() const;

  double processingTimeSeconds() const noexcept;

  const std::string theRetCode;
  const std::string theOutput;
  std::string       theResponder;
  unsigned int      theProcessingTime;
  std::string       theDataOut;
  unsigned short    theLoad1;
  unsigned short    theLoad10;
  unsigned short    theLoad30;
  unsigned int      theHops;
};

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                     aStream,
                         const uiiit::edge::LambdaRequest& aReq);
std::ostream& operator<<(std::ostream&                      aStream,
                         const uiiit::edge::LambdaResponse& aRep);
