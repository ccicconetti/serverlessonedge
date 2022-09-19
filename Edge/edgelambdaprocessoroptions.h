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

#include "Support/clioptions.h"

namespace uiiit {
namespace edge {

/**
 * Specialization of CLI options for edge lambda processors (router,
 * dispatcher).
 */
class EdgeLambdaProcessorOptions final : public support::CliOptions
{
 public:
  EdgeLambdaProcessorOptions(
      int                                          argc,
      char**                                       argv,
      boost::program_options::options_description& aDesc);

  //! \return the edge server end-point.
  const std::string& serverEndpoint() const noexcept {
    return theServerEndpoint;
  }
  //! \return the controller end-point.
  const std::string& controllerEndpoint() const noexcept {
    return theControllerEndpoint;
  }
  //! \return the number of threads.
  size_t numThreads() const noexcept {
    return theNumThreads;
  }
  //! \return the forwarding table configuration end-point.
  const std::string& forwardingEndpoint() const noexcept {
    return theForwardingEndpoint;
  }
  //! \return the router configuration options.
  const std::string& routerConf() const noexcept {
    return theRouterConf;
  }
  //! \return the number of fake lambda functions.
  size_t fakeNumLambdas() const noexcept {
    return theFakeNumLambdas;
  }
  //! \return the number of fake destinations.
  size_t fakeNumDestinations() const noexcept {
    return theFakeNumDestinations;
  }
  //! \return true if SSL/TLS should be used to authenticate the server.
  bool secure() const noexcept;

 private:
  std::string theServerEndpoint;
  std::string theControllerEndpoint;
  size_t      theNumThreads;
  std::string theForwardingEndpoint;
  std::string theRouterConf;
  size_t      theFakeNumLambdas;
  size_t      theFakeNumDestinations;
};

} // namespace edge
} // namespace uiiit
