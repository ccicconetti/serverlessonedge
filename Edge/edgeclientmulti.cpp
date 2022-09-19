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

#include "edgeclientmulti.h"

#include "Edge/edgeclientfactory.h"
#include "Edge/edgeclientgrpc.h"
#include "Support/random.h"
#include "Support/tostring.h"

#ifdef WITH_QUIC
#include "Quic/edgeclientquic.h"
#endif

#include <cassert>
#include <stdexcept>

#include <glog/logging.h>

namespace uiiit {
namespace edge {

EdgeClientMulti::EdgeClientMulti(const std::set<std::string>& aServerEndpoints,
                                 const bool                   aSecure,
                                 const support::Conf&         aClientConf)
    : EdgeClientInterface()
    , thePersistenceProb(aClientConf.getDouble("persistence"))
    , theDesc(aServerEndpoints.size())
    , theConsumer()
    , theConsumerQueue()
    , theQueueOut()
    , thePendingRequest(nullptr)
    , thePrimary(0) {
  if (aClientConf.getDouble("persistence") < 0 or
      aClientConf.getDouble("persistence") > 1) {
    throw std::runtime_error(
        "Invalid configuration: persistence probability (" +
        std::to_string(aClientConf.getDouble("persistence")) +
        ") cannot be < 0 or > 1");
  }

  if (aServerEndpoints.empty()) {
    throw std::runtime_error("Empty set of destinations");
  }

  // start executors
  size_t     i            = 0;
  const auto myClientType = aClientConf("type");
  for (const auto& myEndpoint : aServerEndpoints) {
    auto& myDesc       = theDesc[i];
    myDesc.theIndex    = i;
    myDesc.theEndpoint = myEndpoint;

    if (myClientType == "grpc") {
      myDesc.theClient.reset(new EdgeClientGrpc(myEndpoint, aSecure));
#ifdef WITH_QUIC
    } else if (myClientType == "quic") {
      myDesc.theClient.reset(new EdgeClientQuic(
          QuicParamsBuilder::buildClientHQParams(aClientConf, myEndpoint)));
#endif
    } else {
      throw std::runtime_error("Unknown client type: " + myClientType);
    }

    myDesc.theThread = std::thread([this, &myDesc]() {
      while (execLambda(myDesc)) {
        // do nothing
      }
    });
    i++;
  }
  assert(theDesc.size() == aServerEndpoints.size());

  // start consumer
  theConsumer = std::thread([this]() {
    while (consume()) {
      // do nothing
    }
  });

  // unblock the calling thread upon first lambda call
  theCallingQueue.push(true);

  LOG(INFO) << "starting an edge multi-client towards ["
            << toString(aServerEndpoints, ",")
            << "] with persistence probability "
            << aClientConf.getDouble("persistence");
}

EdgeClientMulti::~EdgeClientMulti() {
  // force termination of the consumer thread
  theConsumerQueue.push({});

  // force termination of the calling thread
  theCallingQueue.push(false);

  // force termination of the executors
  for (auto& myDesc : theDesc) {
    myDesc.theQueueIn.push(MessageIn());
  }

  // join threads
  assert(theConsumer.joinable());
  theConsumer.join();
  for (auto& myDesc : theDesc) {
    assert(myDesc.theThread.joinable());
    myDesc.theThread.join();
  }
}

bool EdgeClientMulti::consume() {
  // get the set of pending clients
  auto myPending = theConsumerQueue.pop();

  // empty set means we are terminating
  if (myPending.empty()) {
    return false;
  }

  // wait for all edge clients to return
  while (not myPending.empty()) {
    auto myResponse = theQueueOut.pop();

    VLOG_IF(2, myResponse) << "non-fastest executor "
                           << theDesc[myResponse.theIndex].theEndpoint
                           << " replied with "
                           << myResponse.theResponse->toString();

    assert(myPending.count(myResponse.theIndex) == 1);
    myPending.erase(myResponse.theIndex);
  }

  // unblock the calling thread when done
  theCallingQueue.push(true);

  return true; // go on buddy
}

bool EdgeClientMulti::execLambda(Desc& aDesc) {
  try {
    // wait for a new lambda request to arrive
    const auto myMessageIn = aDesc.theQueueIn.pop();

    // if the lambda request is empty then we are terminating
    if (not myMessageIn) {
      return false;
    }

    // execute the lambda request and push the message
    try {
      theQueueOut.push(MessageOut(
          aDesc.theIndex,
          std::make_unique<LambdaResponse>(aDesc.theClient->RunLambda(
              *myMessageIn.theRequest, myMessageIn.theDry))));
    } catch (...) {
      // an error occurred, we push an empty response
      // to be handled by the other threads in this object
      theQueueOut.push(MessageOut(aDesc.theIndex));
    }

  } catch (const support::QueueClosed&) {
    // communication channel has been closed, we must terminate
    return false;
  }
  return true;
}

LambdaResponse EdgeClientMulti::RunLambda(const LambdaRequest& aReq,
                                          const bool           aDry) {
  // wait until the consumer is done
  const auto myGoAhead = theCallingQueue.pop();
  if (not myGoAhead) {
    return LambdaResponse("terminating", "");
  }

  // find which clients should be reached in addition to the primary
  // destination
  auto myPending = secondary();

  // reach out for the primary, too
  myPending.insert(thePrimary);

  // create a local copy of the lambda request, which may not survive
  // when this function returns (but some threads may still use it)
  thePendingRequest = std::make_unique<LambdaRequest>(aReq.copy());

  // unblock the selected clients
  assert(not myPending.empty());
  for (const auto ndx : myPending) {
    assert(thePendingRequest.get() != nullptr);
    theDesc[ndx].theQueueIn.push(MessageIn(thePendingRequest.get(), aDry));
  }

  // wait for the fastest client
  MessageOut myFastest(thePrimary);
  while (not myPending.empty()) {
    myFastest = theQueueOut.pop();
    assert(myPending.count(myFastest.theIndex) == 1);
    myPending.erase(myFastest.theIndex);
    if (myFastest and myFastest.theResponse->theRetCode == "OK") {
      // good response, we may proceed after recording who responded
      myFastest.theResponse->theResponder =
          theDesc[myFastest.theIndex].theEndpoint;
      break;
    }
  }

  // none of the destinations worked out
  if (not myFastest) {
    assert(myPending.empty());
    theCallingQueue.push(true);
    return LambdaResponse("none of the destinations responded correctly", "");
  }

  // only non-OK responses
  if (myFastest.theResponse->theRetCode != "OK") {
    assert(myPending.empty());
    theCallingQueue.push(true);
    return *myFastest.theResponse;
  }

  // pass the pending set to the consumer only if it is non-empty
  if (myPending.empty()) {
    // do not wait at all
    theCallingQueue.push(true);
  } else {
    theConsumerQueue.push(myPending);
  }

  // the fastest executor becomes the new primary
  thePrimary = myFastest.theIndex;

  VLOG(2) << "fastest executor " << theDesc[myFastest.theIndex].theEndpoint
          << " replied with " << myFastest.theResponse->toString();

  // return the response while there may be other threads working
  assert(myFastest.theResponse);
  return *myFastest.theResponse;
}

std::set<size_t> EdgeClientMulti::secondary() {
  std::set<size_t> ret;
  for (size_t i = 0; i < theDesc.size(); i++) {
    if (i == thePrimary) {
      continue;
    }
    if (support::random() < thePersistenceProb) {
      ret.insert(i);
    }
  }
  return ret;
}

} // namespace edge
} // namespace uiiit
