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

#include <array>
#include <functional>
#include <iostream>
#include <memory>

namespace uiiit {
namespace edge {

class Processor;
struct LambdaRequest;
struct LambdaResponse;

/**
 * How many operations and how much memory is required for the execution of
 * a given lambda and  processor.
 */
struct LambdaRequirements {
  uint64_t theOperations;
  uint64_t theMemory; // in bytes
};

/**
 * Return computational requirements, both time and space, proportional to the
 * input size, irrespective of the processor on which the lambda is executed.
 */
class ProportionalRequirements
{
 public:
  explicit ProportionalRequirements(const float aOpCoeff,
                                    const float aOpOffset,
                                    const float aMemCoeff,
                                    const float aMemOffset) noexcept;

  LambdaRequirements operator()(const Processor&     aProc,
                                const LambdaRequest& aReq) const noexcept;

 private:
  const float theOpCoeff;
  const float theOpOffset;
  const float theMemCoeff;
  const float theMemOffset;
};

/**
 * Return fixed computational requirements.
 */
class FixedRequirements
{
 public:
  explicit FixedRequirements(const uint64_t aOperations,
                             const uint64_t aMemory) noexcept
      : theRequirements{aOperations, aMemory} {
  }
  LambdaRequirements operator()(const Processor&     aProc,
                                const LambdaRequest& aReq) const noexcept {
    return theRequirements;
  }

 private:
  LambdaRequirements theRequirements;
};

/**
 * Abstraction of a computation function.
 */
class Lambda final
{
 public:
  using Converter =
      std::function<LambdaRequirements(const Processor&, const LambdaRequest&)>;

  /**
   * Lambda function that copies the input to the output.
   *
   * \param aName lambda function name.
   * \param aConverter function to determine the number of operations and memory
   *        required for a given lambda request.
   */
  explicit Lambda(const std::string& aName,
                  const Converter&   aConverter) noexcept;

  /**
   * Lambda function returning always the given output.
   *
   * \param aName lambda function name.
   * \param aOutput output produced by the lambda function.
   * \param aConverter function to determine the number of operations and memory
   *        required for a given lambda request.
   */
  explicit Lambda(const std::string& aName,
                  const std::string& aOutput,
                  const Converter&   aConverter) noexcept;

  /**
   * \param aProc processor on which the lambda is executed.
   * \param aReq lamdba request executed.
   */
  LambdaRequirements requirements(const Processor&     aProc,
                                  const LambdaRequest& aReq) const;

  /**
   * \param aReq The lambda request.
   * \param aLoads The processor loads before the execution of the request.
   *
   * \return a newly created lambda response.
   */
  std::shared_ptr<const LambdaResponse>
  execute(const LambdaRequest& aReq, const std::array<double, 3>& aLoads) const;

  // accessors
  std::string name() const noexcept {
    return theName;
  }

 private:
  explicit Lambda(const std::string& aName,
                  const bool         aCopyInput,
                  const std::string& aOutput,
                  const Converter&   aConverter) noexcept;

 private:
  const std::string theName;
  const bool        theCopyInput;
  const std::string theOutput;
  const Converter   theConverter;
};

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                          aStream,
                         const uiiit::edge::LambdaRequirements& aReq);
std::ostream& operator<<(std::ostream&              aStream,
                         const uiiit::edge::Lambda& aLambda);
