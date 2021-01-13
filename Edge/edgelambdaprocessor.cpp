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

#include "edgelambdaprocessor.h"

#include "Support/conf.h"
#include "Support/random.h"
#include "edgecontrollerclient.h"
#include "edgemessages.h"

#include <glog/logging.h>

#include <grpc++/grpc++.h>

#include <chrono>
#include <thread>

namespace uiiit {
namespace edge {

EdgeLambdaProcessor::EdgeLambdaProcessor(const std::string& aLambdaEndpoint,
                                         const std::string& aCommandsEndpoint,
                                         const std::string& aControllerEndpoint,
                                         const support::Conf& aRouterConf,
                                         const support::Conf& aQuicServerConf)
    : EdgeServer(aLambdaEndpoint)
    , theCommandsEndpoint(aCommandsEndpoint)
    , theControllerEndpoint(aControllerEndpoint)
    , theFakeProcessor(aRouterConf.count("fake") > 0 and
                       aRouterConf.getBool("fake"))
    , theClientPool(aQuicServerConf, aRouterConf.getUint("max-pending-clients"))
    , theControllerClient(aControllerEndpoint.empty() ?
                              nullptr :
                              new EdgeControllerClient(aControllerEndpoint))
    , theRandomWaiter(aRouterConf.getDouble("min-forward-time"),
                      aRouterConf.getDouble("max-forward-time")) {
  LOG(INFO) << "Created an EdgeLambdaProcessor with max-pending-clients "
            << aRouterConf.getUint("max-pending-clients") << ", forward-time ["
            << (aRouterConf.getDouble("min-forward-time") * 1e3) << ","
            << (aRouterConf.getDouble("max-forward-time") * 1e3) << "] ms";
  LOG_IF(WARNING, not aControllerEndpoint.empty() and aCommandsEndpoint.empty())
      << "No edge router specified";
  LOG_IF(INFO, aControllerEndpoint.empty())
      << "No controller specified: announce disabled";
  LOG_IF(INFO, theFakeProcessor) << "FAKE edge lambda processor configuration";
}

void EdgeLambdaProcessor::init() {
  if (theControllerClient) {
    LOG(INFO) << "Announcing to " << theControllerEndpoint;
    controllerCommand([this](EdgeControllerClient& aClient) {
      aClient.announceRouter(theServerEndpoint, theCommandsEndpoint);
    });
  }
}

EdgeLambdaProcessor::~EdgeLambdaProcessor() {
}

std::string EdgeLambdaProcessor::defaultConf() {
  return "max-pending-clients=2,min-forward-time=0,max-forward-time=0";
}

rpc::LambdaResponse
EdgeLambdaProcessor::process(const rpc::LambdaRequest& aReq) {
  std::string myRetCode        = "OK";
  auto        myNoDestinations = false;

  while (not myNoDestinations) {
    VLOG(3) << LambdaRequest(aReq).toString();
    if (aReq.hops() > 254) { // loop detection
      myRetCode = "loop detected";
      break;
    }
    std::string myDestination;
    try {
      myDestination = destination(aReq);

      theRandomWaiter();

      // if this is fake processor then we do not contact the next
      // destination, but rather return immediately a fake OK response
      const auto ret =
          theFakeProcessor ?
              std::make_pair(LambdaResponse("OK", ""), 0.001 + random()) :
              theClientPool(myDestination, LambdaRequest(aReq), false);

      myRetCode = ret.first.theRetCode;

      if (myRetCode == "OK") {
        processSuccess(aReq, myDestination, ret.first, ret.second);
        return ret.first.toProtobuf();
      }

    } catch (const std::exception& aErr) {
      myRetCode = aErr.what();
    } catch (...) {
      myRetCode = "Unknown error";
    }

    // purge this entry from both the local table and the controller if there
    // have been errors
    assert(myRetCode != "OK");
    if (not myDestination.empty()) {
      processFailure(aReq, myDestination);
      controllerCommand([&myDestination](EdgeControllerClient& aClient) {
        aClient.removeComputer(myDestination);
      });
    } else {
      myNoDestinations = true;
    }
  }

  rpc::LambdaResponse myResp;
  myResp.set_retcode(myRetCode);
  return myResp;
}

void EdgeLambdaProcessor::controllerCommand(
    const std::function<void(EdgeControllerClient&)>& aCommand) noexcept {
  if (not theControllerClient) {
    return;
  }

  try {
    aCommand(*theControllerClient);
  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Could not reach controller at " << theControllerEndpoint
               << ": " << aErr.what();
  } catch (...) {
    LOG(ERROR) << "Unknown error when reaching the controller at "
               << theControllerEndpoint;
  }
}

EdgeLambdaProcessor::RandomWaiter::RandomWaiter(const double aMin,
                                                const double aMax)
    : theMin(aMin)
    , theSpan(aMax - aMin) {
  assert(aMax >= aMin);
}

void EdgeLambdaProcessor::RandomWaiter::operator()() const {
  if (theMin == 0 and theSpan == 0) {
    return;
  }

  const auto myRndTime = theMin + theSpan * uiiit::support::random();
  std::this_thread::sleep_for(
      std::chrono::microseconds(static_cast<long>(0.5 + myRndTime * 1e6)));
}

} // namespace edge
} // namespace uiiit
