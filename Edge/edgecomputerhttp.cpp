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

#include "Edge/edgecomputerhttp.h"

#include "Edge/edgemessages.h"
#include "Rest/client.h"

#include <exception>
#include <glog/logging.h>
#include <grpc++/grpc++.h>
#include <stdexcept>

namespace uiiit {
namespace edge {

EdgeComputerHttp::EdgeComputerHttp(const size_t       aNumThreads,
                                   const std::size_t  aNumHttpClients,
                                   const std::string& aServerEndpoint,
                                   const bool         aSecure,
                                   const Type         aType,
                                   const std::string& aGatewayUrl)
    : EdgeComputer(aNumThreads, aServerEndpoint, aSecure)
    , theNextId(0)
    , theWorkers()
    , theJobs() {
  if (aNumHttpClients == 0) {
    throw std::runtime_error("invalid vanishing number of HTTP clients");
  }
  for (std::size_t i = 0; i < aNumHttpClients; i++) {
    theWorkers.add(
        std::make_unique<Worker>(*this, aType, aGatewayUrl, theJobs));
  }
  theWorkers.start();
}

EdgeComputerHttp::EdgeComputerHttp(const std::size_t  aNumHttpClients,
                                   const std::string& aServerEndpoint,
                                   const bool         aSecure,
                                   const Type         aType,
                                   const std::string& aGatewayUrl)
    : EdgeComputerHttp(
          0, aNumHttpClients, aServerEndpoint, aSecure, aType, aGatewayUrl) {
  // noop
}

EdgeComputerHttp::~EdgeComputerHttp() {
  theJobs.close();
  theWorkers.stop();
  theWorkers.wait();
}

uint64_t EdgeComputerHttp::realExecution(const rpc::LambdaRequest& aRequest) {
  const auto aId = theNextId++;
  theJobs.push(Job{aId, aRequest});
  return aId;
}

double EdgeComputerHttp::dryExecution(
    [[maybe_unused]] const rpc::LambdaRequest& aRequest,
    [[maybe_unused]] std::array<double, 3>&    aLastUtils) {
  return 0;
}

std::string toString(const EdgeComputerHttp::Type aType) {
  switch (aType) {
    case EdgeComputerHttp::Type::OPENFAAS_0_8:
      return "OpenFaaS(0.8)";
    default:
      return "unknown";
  }
  assert(false);
}

EdgeComputerHttp::Type
edgeComputerHttpTypeFromString(const std::string& aType) {
  if (aType == "OpenFaaS(0.8)") {
    return EdgeComputerHttp::Type::OPENFAAS_0_8;
  }
  throw std::runtime_error("unknown EdgeComputerHttp type: " + aType);
}

EdgeComputerHttp::Worker::Worker(EdgeComputerHttp&    aParent,
                                 const Type           aType,
                                 const std::string&   aGatewayUrl,
                                 support::Queue<Job>& aQueue)
    : theParent(aParent)
    , theType(aType)
    , theClient(std::make_unique<rest::Client>(aGatewayUrl, false))
    , theQueue(aQueue) {
  LOG(INFO) << "EdgeComputerHttp worker created, type " << toString(aType)
            << ", gateway-url " << aGatewayUrl;
}

void EdgeComputerHttp::Worker::operator()() {
  while (true) {
    std::string myRetCode = "OK";
    uint64_t    myId;
    try {
      const auto myJob = theQueue.pop();
      myId             = myJob.theId;
      if (theType == Type::OPENFAAS_0_8) {
        const auto myResult = theClient->post(
            myJob.theRequest.input(), "function/" + myJob.theRequest.name());

        if (myResult.first != web::http::status_codes::OK) {
          myRetCode = "error HTTP response received (" +
                      std::to_string(myResult.first) + ")";
        } else {
          theParent.taskDone(
              myId,
              std::make_shared<const LambdaResponse>("OK", myResult.second));
        }
      }
    } catch (const support::QueueClosed&) {
      break;
    } catch (const std::exception& aErr) {
      myRetCode = std::string("exception caught: ") + aErr.what();
    } catch (...) {
      myRetCode = "unknown exception caught";
    }
    if (myRetCode != "OK") {
      theParent.taskDone(myId,
                         std::make_shared<const LambdaResponse>(myRetCode, ""));
    }
  }
}

void EdgeComputerHttp::Worker::stop() {
  VLOG(1) << "EdgeComputerHttp worker stopped";
}

} // namespace edge
} // namespace uiiit
