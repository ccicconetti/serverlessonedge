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

#include "Edge/edgecomputer.h"
#include "Support/queue.h"
#include "Support/threadpool.h"

namespace uiiit {

namespace rest {
class Client;
}

namespace edge {

/**
 * @brief EdgeComputer that invokes lambda functions through HTTP commands.
 *
 * The load measurement is not defined for this type of EdgeComputer.
 */
class EdgeComputerHttp final : public EdgeComputer
{
 public:
  enum class Type : uint16_t {
    // https://github.com/openfaas/faas/blob/0.27.1/api-docs/spec.openapi.yml
    OPENFAAS_0_8 = 1,
  };

  /**
   * Create an edge computer that invokes functions through
   * HTTP commands and supports asynchronous calls.
   *
   * \param aNumThreads the number of threads needed for asynchronous responses.
   *
   * \param aNumHttpClients the number of HTTP clients.
   *
   * \param aServerEndpoint the listening end-point of this server.
   *
   * \param aSecure if true use SSL/TLS authentication.
   *
   * \param aType the HTTP API to use.
   *
   * \param aGatewayUrl the URL of the gateway to invoke functions.
   *
   * The companion end-point is empty by default. If needed, i.e., if this
   * edge computer is expected to serve function chains or DAGs, then it must be
   * set via the companion() method.
   *
   * \throw std::runtime_error if aNumHttpClients is 0
   */
  explicit EdgeComputerHttp(const size_t       aNumThreads,
                            const std::size_t  aNumHttpClients,
                            const std::string& aServerEndpoint,
                            const bool         aSecure,
                            const Type         aType,
                            const std::string& aGatewayUrl);

  //! Create an edge computer that invokes functions
  //! through HTTP commands that only supports synchronous calls.
  explicit EdgeComputerHttp(const std::size_t  aNumHttpClients,
                            const std::string& aServerEndpoint,
                            const bool         aSecure,
                            const Type         aType,
                            const std::string& aGatewayUrl);

  ~EdgeComputerHttp() override;

 private:
  uint64_t realExecution(const rpc::LambdaRequest& aRequest) override;

  double dryExecution(const rpc::LambdaRequest& aRequest,
                      std::array<double, 3>&    aLastUtils) override;

  struct Job {
    uint64_t           theId;
    rpc::LambdaRequest theRequest;
  };

  class Worker final
  {
   public:
    explicit Worker(EdgeComputerHttp&    aParent,
                    const Type           aType,
                    const std::string&   aGatewayUrl,
                    support::Queue<Job>& aQueue);
    void operator()();
    void stop();

   private:
    EdgeComputerHttp&                   theParent;
    const Type                          theType;
    const std::unique_ptr<rest::Client> theClient;
    support::Queue<Job>&                theQueue;
  };

  std::atomic<uint64_t>                        theNextId;
  support::ThreadPool<std::unique_ptr<Worker>> theWorkers;
  support::Queue<Job>                          theJobs;
};

std::string            toString(const EdgeComputerHttp::Type aType);
EdgeComputerHttp::Type edgeComputerHttpTypeFromString(const std::string& aType);

} // end namespace edge
} // end namespace uiiit
