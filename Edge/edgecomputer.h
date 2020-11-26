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

#include "Support/chrono.h"
#include "computer.h"
#include "edgeserver.h"

namespace uiiit {
namespace edge {

class EdgeComputer final : public EdgeServer
{
  /**
   * Descriptor for a pending lambda request.
   *
   * Created as a new lambda request arrives, destroyed immediately after the
   * lambda response is generated. A chronometer is started automatically upon
   * creation.
   */
  struct Descriptor {
    explicit Descriptor()
        : theCondition()
        , theResponse()
        , theDone(false)
        , theChrono(true) {
    }
    std::condition_variable               theCondition;
    std::shared_ptr<const LambdaResponse> theResponse;
    bool                                  theDone;
    support::Chrono                       theChrono;
  };

  using UtilCallback = Computer::UtilCallback;

 public:
  /**
   * Create an edge server with a given number of threads.
   *
   * \param aServerEndpoint the listening end-point of this server.
   * \param aCallback the function called as new load values are available.
   */
  explicit EdgeComputer(const std::string&  aServerEndpoint,
                        const UtilCallback& aCallback);

  //! \return The computer inside this edge server.
  Computer& computer() {
    return theComputer;
  }

 private:
  //! Callback invoked by the computer once a task is complete.
  void taskDone(const uint64_t                               aId,
                const std::shared_ptr<const LambdaResponse>& aResponse);

  //! Perform actual processing of a lambda request.
  rpc::LambdaResponse process(const rpc::LambdaRequest& aReq) override;

 private:
  Computer                                        theComputer;
  std::condition_variable                         theDescriptorsCv;
  std::map<uint64_t, std::unique_ptr<Descriptor>> theDescriptors;
};

} // end namespace edge
} // end namespace uiiit
