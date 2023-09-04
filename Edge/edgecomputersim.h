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

#include "Edge/computer.h"
#include "Edge/edgecomputer.h"

namespace uiiit {
namespace edge {

/**
 * @brief EdgeComputer that simulates the invocation of lambda functions.
 */
class EdgeComputerSim final : public EdgeComputer
{

  using UtilCallback = Computer::UtilCallback;

 public:
  /**
   * Create an edge computer that support asynchronous calls.
   *
   * \param aNumThreads the number of threads needed for asynchronous responses.
   *
   * \param aServerEndpoint the listening end-point of this server.
   *
   * \param aSecure if true use SSL/TLS authentication.
   *
   * \param aCallback the function called as new load values are available.
   *
   * The companion end-point is empty by default. If needed, i.e., if this
   * edge computer is expected to serve function chains or DAGs, then it must be
   * set via the companion() method.
   */
  explicit EdgeComputerSim(const size_t        aNumThreads,
                           const std::string&  aServerEndpoint,
                           const bool          aSecure,
                           const UtilCallback& aCallback);

  //! Create an edge computer that only supports synchronous calls.
  explicit EdgeComputerSim(const std::string&  aServerEndpoint,
                           const bool          aSecure,
                           const UtilCallback& aCallback);

  //! \return The computer inside this edge server.
  Computer& computer() {
    return theComputer;
  }

 private:
  uint64_t realExecution(const rpc::LambdaRequest& aRequest) override;

  double dryExecution(const rpc::LambdaRequest& aRequest,
                      std::array<double, 3>&    aLastUtils) override;

  Computer theComputer;
};

} // end namespace edge
} // end namespace uiiit
