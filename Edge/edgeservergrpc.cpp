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

#include "edgeservergrpc.h"

#include "Support/chrono.h"
#include "edgemessages.h"

#include <exception>
#include <glog/logging.h>
#include <stdexcept>

// #define TRACE_TASKS

GPRAPI void gpr_log(const char*, int, gpr_log_severity, const char*, ...) {
}

namespace uiiit {
namespace edge {

EdgeServerGrpc::CallData::CallData(rpc::EdgeServer::AsyncService* aService,
                                   grpc::ServerCompletionQueue*   aCq,
                                   EdgeServerGrpc&                aEdgeServer)
    : theService(aService)
    , theCq(aCq)
    , theEdgeServer(aEdgeServer)
    , theContext()
    , theRequest()
    , theResponse()
    , theResponder(&theContext)
    , theStatus(CREATE) {
  // Invoke the serving logic right away.
  Proceed();
}

void EdgeServerGrpc::CallData::Proceed() {
  if (theStatus == CREATE) {
    VLOG(2) << "CREATE";

    // Make this instance progress to the PROCESS state.
    theStatus = PROCESS;

    // As part of the initial CREATE state, we *request* that the system
    // start processing RunLambda requests. In this request, "this" acts are
    // the tag uniquely identifying the request (so that different CallData
    // instances can serve different requests concurrently), in this case
    // the memory address of this CallData instance.
    theService->RequestRunLambda(
        &theContext, &theRequest, &theResponder, theCq, theCq, this);

  } else if (theStatus == PROCESS) {
    VLOG(2) << "PROCESS (" << theContext.peer() << ")";

#ifdef TRACE_TASKS
    support::Chrono myChrono(true);
#endif

    // Spawn a new CallData instance to serve new clients while we process
    // the one for this CallData. The instance will deallocate itself as
    // part of its FINISH state.
    new CallData(theService, theCq, theEdgeServer);

    // The actual processing.
    try {
      theResponse = theEdgeServer.process(theRequest);
    } catch (const std::exception& aErr) {
      theResponse.set_retcode("invalid '" + theRequest.name() +
                              "' request: " + aErr.what());
    } catch (...) {
      theResponse.set_retcode("invalid '" + theRequest.name() +
                              "' request: unknown reasons");
    }

#ifdef TRACE_TASKS
    std::cout << theRequest.name() << " took " << myChrono.stop()
              << " return-code " << theResponse.retcode() << std::endl;
#endif

    // And we are done! Let the gRPC runtime know we've finished, using the
    // memory address of this instance as the uniquely identifying tag for
    // the event.
    theStatus = FINISH;
    theResponder.Finish(theResponse, grpc::Status::OK, this);

  } else {
    VLOG(2) << "FINISH";
    assert(theStatus == FINISH);

    // Once in the FINISH state, deallocate ourselves (CallData).
    delete this;
  }
}

EdgeServerGrpc::EdgeServerGrpc(EdgeServer&        aEdgeServer,
                               const std::string& aServerEndpoint,
                               const size_t       aNumThreads)
    : EdgeServerImpl(aEdgeServer)
    , theMutex()
    , theServerEndpoint(aServerEndpoint)
    , theNumThreads(aNumThreads)
    , theCq()
    , theService()
    , theServer()
    , theHandlers() {
  if (aNumThreads == 0) {
    throw std::runtime_error("Cannot spawn 0 threads");
  }
  if (aServerEndpoint.empty()) {
    throw std::runtime_error("Invalid request to bind to an empty end-point");
  }
}

void EdgeServerGrpc::run() {
  if (not theHandlers.empty()) {
    throw std::runtime_error("Method run() must be invoked only once");
  }

  grpc::ServerBuilder myBuilder;
  myBuilder.AddListeningPort(theServerEndpoint,
                             grpc::InsecureServerCredentials());
  myBuilder.RegisterService(&theService);
  theCq     = myBuilder.AddCompletionQueue();
  theServer = myBuilder.BuildAndStart();
  if (theServer == nullptr) {
    throw std::runtime_error("Could not bind to: " + theServerEndpoint);
  }
  LOG(INFO) << "Server listening on " << theServerEndpoint << " (spawning "
            << theNumThreads << " threads)";

  for (size_t i = 0; i < theNumThreads; i++) {
    theHandlers.emplace_back(std::thread([this]() { handle(); }));
  }

  theEdgeServer.init(threadIds());
}

void EdgeServerGrpc::wait() {
  if (theServer) {
    theServer->Wait();
  } else {
    throw std::runtime_error(
        "Cannot wait for termination of a server that has not been started");
  }
}

EdgeServerGrpc::~EdgeServerGrpc() {
  if (theServer) {
    assert(theCq);
    assert(not theHandlers.empty());
    theServer->Shutdown();
    theCq->Shutdown();
    for (auto& myHandler : theHandlers) {
      myHandler.join();
    }
  }
}

void EdgeServerGrpc::handle() {
  // Spawn a new CallData instance to serve new clients.
  new CallData(&theService, theCq.get(), *this);
  void* myTag; // uniquely identifies a request.
  bool  myOk;
  while (true) {
    // Block waiting to read the next event from the completion queue. The
    // event is uniquely identified by its tag, which in this case is the
    // memory address of a CallData instance.
    // The return value of Next should always be checked. This return value
    // tells us whether there is any kind of event or the completion queue is
    // shutting down.
    if (not theCq->Next(&myTag, &myOk)) {
      LOG(WARNING) << "terminating (ok = " << myOk << ")";
      break;
    }

    if (not myOk) {
      LOG(WARNING) << "Invalid event found";
    } else {
      static_cast<CallData*>(myTag)->Proceed();
    }
  }
}

rpc::LambdaResponse EdgeServerGrpc::process(const rpc::LambdaRequest& aReq) {
  return theEdgeServer.process(aReq);
}

std::set<std::thread::id> EdgeServerGrpc::threadIds() const {
  std::set<std::thread::id> ret;
  for (const auto& myThread : theHandlers) {
    ret.insert(myThread.get_id());
  }
  return ret;
}

} // namespace edge
} // namespace uiiit
