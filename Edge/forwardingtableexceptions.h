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

#include <stdexcept>
#include <string>

namespace uiiit {
namespace edge {

struct NoDestinations : public std::runtime_error {
  explicit NoDestinations(const std::string& aEntry)
      : std::runtime_error("No destinations found for the entry: " + aEntry) {
  }
  explicit NoDestinations()
      : std::runtime_error("No destinations found for the entry") {
  }
};

struct InvalidDestination : public std::runtime_error {
  explicit InvalidDestination(const std::string& aLambda,
                              const std::string& aDest)
      : std::runtime_error("Invalid destination towards '" + aDest +
                           "' for lambda function '" + aLambda + "'") {
  }

  explicit InvalidDestination(const std::string& aDest, const float aWeight)
      : std::runtime_error("Invalid destination towards '" + aDest +
                           "' with weight " + std::to_string(aWeight)) {
  }
};

struct InvalidWeight : public std::runtime_error {
  explicit InvalidWeight(const float aWeight)
      : std::runtime_error("Invalid weight '" + std::to_string(aWeight) +
                           "', must be > 0") {
  }
};

struct InvalidWeightFactor : public std::runtime_error {
  explicit InvalidWeightFactor(const float aFactor)
      : std::runtime_error("Invalid weight factor '" + std::to_string(aFactor) +
                           "', must be > 0") {
  }
};

struct InvalidForwardingTableType : public std::runtime_error {
  explicit InvalidForwardingTableType(const std::string& aType)
      : std::runtime_error("Invalid forwarding table type '" + aType + "'") {
  }
};

} // namespace edge
} // namespace uiiit
