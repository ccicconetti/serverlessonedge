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

#include "lambda.h"

#include "edgemessages.h"
#include "processor.h"

#include <functional>
#include <iostream>
#include <limits>

namespace uiiit {
namespace edge {

ProportionalRequirements::ProportionalRequirements(
    const float aOpCoeff,
    const float aOpOffset,
    const float aMemCoeff,
    const float aMemOffset) noexcept
    : theOpCoeff(aOpCoeff)
    , theOpOffset(aOpOffset)
    , theMemCoeff(aMemCoeff)
    , theMemOffset(aMemOffset) {
}

LambdaRequirements
ProportionalRequirements::operator()(const Processor&     aProc,
                                     const LambdaRequest& aReq) const noexcept {
  std::ignore            = aProc;
  const auto myInputSize = aReq.theInput.size();
  return LambdaRequirements{
      static_cast<uint64_t>(
          std::max<int64_t>(0, 0.5 + theOpOffset + theOpCoeff * myInputSize)),
      static_cast<uint64_t>(std::max<int64_t>(
          0, 0.5 + theMemOffset + theMemCoeff * myInputSize))};
}

Lambda::Lambda(const std::string& aName, const Converter& aConverter) noexcept
    : Lambda(aName, true, std::string(), aConverter) {
}

Lambda::Lambda(const std::string& aName,
               const std::string& aOutput,
               const Converter&   aConverter) noexcept
    : Lambda(aName, false, aOutput, aConverter) {
}

Lambda::Lambda(const std::string& aName,
               const bool         aCopyInput,
               const std::string& aOutput,
               const Converter&   aConverter) noexcept
    : theName(aName)
    , theCopyInput(aCopyInput)
    , theOutput(aOutput)
    , theConverter(aConverter) {
}

LambdaRequirements Lambda::requirements(const Processor&     aProc,
                                        const LambdaRequest& aReq) const {
  return theConverter(aProc, aReq);
}

std::shared_ptr<const LambdaResponse>
Lambda::execute(const LambdaRequest&         aReq,
                const std::array<double, 3>& aLoads) const {
  return std::make_shared<const LambdaResponse>(
      "OK", theCopyInput ? aReq.theInput : theOutput, aLoads);
}

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                          aStream,
                         const uiiit::edge::LambdaRequirements& aReq) {
  aStream << aReq.theOperations << " operations, "
          << "memory " << aReq.theMemory << " bytes";
  return aStream;
}

std::ostream& operator<<(std::ostream&              aStream,
                         const uiiit::edge::Lambda& aLambda) {
  aStream << "name " << aLambda.name();
  return aStream;
}
