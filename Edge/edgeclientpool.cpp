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

#include "edgeclientpool.h"

#include "Edge/edgeclientfactory.h"
#include "Support/chrono.h"
#include "Support/conf.h"

#include <glog/logging.h>

#include <sstream>
#include <stdexcept>

namespace uiiit {
namespace edge {

EdgeClientPool::EdgeClientPool(const support::Conf& aConf,
                               const size_t         aMaxClients)
    : theMaxClients(aMaxClients)
    , theMutex()
    , theConf(aConf)
    , thePool() {
  // noop
}

std::pair<LambdaResponse, double>
EdgeClientPool::operator()(const std::string&   aDestination,
                           const LambdaRequest& aReq,
                           const bool           aDry) {
  debugPrintPool();

  support::Chrono myChrono(true);

  // obtain a client from the pool
  std::unique_ptr<EdgeClientInterface> myClient = getClient(aDestination);
  assert(myClient);

  // execute the lambda function
  const auto myReq  = aReq.makeOneMoreHop();
  auto       myResp = myClient->RunLambda(myReq, aDry);

  // if the lambda does not include the actual responder then we set it to
  // the destination
  if (myResp.theResponder.empty()) {
    myResp.theResponder = aDestination;
  }

  // release the client to the pool
  releaseClient(aDestination, std::move(myClient));

  return std::make_pair(myResp, myChrono.stop());
}

std::unique_ptr<EdgeClientInterface>
EdgeClientPool::getClient(const std::string& aDestination) {
  std::unique_lock<std::mutex> myLock(theMutex);
  auto&                        myDesc = thePool[aDestination]; // may insert
  if (theMaxClients > 0) {
    myDesc.theAvailableCond.wait(myLock, [&myDesc, this]() {
      return not myDesc.theFree.empty() or myDesc.theBusy < theMaxClients;
    });
  }
  if (myDesc.theFree.empty()) {
    assert(theMaxClients == 0 or myDesc.theBusy < theMaxClients);
    myDesc.theBusy++;

    return EdgeClientFactory::make({aDestination}, theConf);
  }
  assert(not myDesc.theFree.empty());
  std::unique_ptr<EdgeClientInterface> myNewClient;
  myNewClient.swap(myDesc.theFree.front());
  myDesc.theFree.pop_front();
  myDesc.theBusy++;
  return myNewClient;
}

void EdgeClientPool::releaseClient(
    const std::string&                     aDestination,
    std::unique_ptr<EdgeClientInterface>&& aClient) {
  const std::lock_guard<std::mutex> myLock(theMutex);
  auto& myDesc = thePool[aDestination]; // may insert
  myDesc.theFree.emplace_back(std::move(aClient));
  myDesc.theBusy--;
  myDesc.theAvailableCond.notify_one();
}

void EdgeClientPool::debugPrintPool() {
  if (VLOG_IS_ON(2)) {
    const std::lock_guard<std::mutex> myLock(theMutex);
    std::stringstream                 myStr;
    size_t                            myCur = 0;
    for (const auto& myDesc : thePool) {
      myStr << "\ndest " << myDesc.first << ", busy " << myDesc.second.theBusy
            << ", free " << myDesc.second.theFree.size();
      myCur += myDesc.second.theFree.size();
    }
    LOG(INFO) << "pool total size " << myCur << ", max " << theMaxClients
              << myStr.str();
  }
}

} // namespace edge
} // namespace uiiit
