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

#include "Edge/Model/chainfactory.h"
#include "Edge/Model/dagfactory.h"
#include "Edge/callbackserver.h"
#include "Edge/stateserver.h"
#include "Simulation/unifclient.h"
#include "Support/chrono.h"
#include "Support/glograii.h"
#include "Support/saver.h"
#include "Support/split.h"
#include "Support/threadpool.h"
#include "Support/tostring.h"

#include <glog/logging.h>

#include <boost/program_options.hpp>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

namespace po = boost::program_options;
namespace ec = uiiit::edge;
namespace es = uiiit::simulation;

class ResponseSaver final
{
 public:
  ResponseSaver(const std::string& aCallback)
      : theQueue()
      , theCallbackServer(aCallback, theQueue)
      , theTerminating(false)
      , theThread()
      , theClient(nullptr) {
    theCallbackServer.run(false);
  }

  ~ResponseSaver() {
    theTerminating = true;
    theQueue.close();
    theThread.join();
  }

  void client(es::Client* aClient) {
    assert(theClient == nullptr);
    assert(aClient != nullptr);
    theClient = aClient;
    theThread = std::thread([this]() { run(); });
  }

 private:
  void run() {
    while (true) {
      try {
        if (theTerminating) {
          return;
        }
        const auto myResponse = theQueue.pop();
        VLOG(3) << "async response received, " << myResponse;
        assert(theClient != nullptr);
        theClient->recordStat(myResponse);
      } catch (const uiiit::support::QueueClosed&) {
        // graceful termination
      } catch (const std::exception& aErr) {
        LOG(ERROR) << "exception thrown: " << aErr.what();
      } catch (...) {
        LOG(ERROR) << "unknown exception thrown";
      }
    }
  }

 private:
  ec::CallbackServer::Queue theQueue;
  ec::CallbackServer        theCallbackServer;
  std::atomic<bool>         theTerminating;
  std::thread               theThread;
  es::Client*               theClient;
};

int main(int argc, char* argv[]) {
  uiiit::support::GlogRaii myGlogRaii(argv[0]);

  std::string myServerEndpoints;
  std::string myClientConf;
  std::string myLambda;
  std::string mySizes;
  std::string myContent;
  std::string myOutputFile;
  std::string myChainConf;
  std::string myDagConf;
  std::string myCallback;
  std::string myStateEndpoint;
  size_t      myDuration;
  size_t      myMaxRequests;
  size_t      myNumThreads;
  double      myInitialDelay;
  double      myInterRequestTime;
  std::string myInterRequestType;
  size_t      mySeedUser;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
    ("help,h", "produce help message")
    ("server-endpoints",
     po::value<std::string>(&myServerEndpoints)->default_value("localhost:6473"),
     "Server end-points (comma-separated list).")
    ("client-conf",
     po::value<std::string>(&myClientConf)->default_value("type=grpc, persistence=0.05"),
     "Configuration of the edge clients.")
    ("lambda",
     po::value<std::string>(&myLambda)->default_value("clambda0"),
     "Lambda function name.")
    ("sizes",
     po::value<std::string>(&mySizes)->default_value("100"),
     "Input sizes, separated by comma. Repeated values admitted.")
    ("content",
     po::value<std::string>(&myContent)->default_value(""),
     "Use the given fixed content for all lambda requests.")
    ("output-file",
     po::value<std::string>(&myOutputFile)->default_value("/dev/stdout"),
     "Output file name. If specified suppresses normal output.")
    ("duration",
     po::value<size_t>(&myDuration)->default_value(0),
     "Experiment duration, in s. 0 means infinite.")
    ("max-requests",
     po::value<size_t>(&myMaxRequests)->default_value(std::numeric_limits<size_t>::max()),
     "Number of requests to issue.")
    ("num-threads",
     po::value<size_t>(&myNumThreads)->default_value(1),
     "Number of threads.")
    ("initial-delay",
     po::value<double>(&myInitialDelay)->default_value(0),
     "Wait for the specified interval before starting.")
    ("inter-request-time",
     po::value<double>(&myInterRequestTime)->default_value(0),
     "Wait for the specified interval before consecutive requests.")
    ("inter-request-type",
     po::value<std::string>(&myInterRequestType)->default_value("constant"),
     "One of: constant, uniform, poisson.")
    ("chain-conf",
     po::value<std::string>(&myChainConf)->default_value(""),
     "Function chain configuration. Load from file with type=file,filename=CHAINFILE.json. Use type=make-template to generated an example. If present override --lambda.")
    ("dag-conf",
     po::value<std::string>(&myDagConf)->default_value(""),
     "Function DAG configuration. Load from file with type=file,filename=DAGFILE.json. Use type=make-template to generated an example. If present override --lambda.")
    ("callback-endpoint",
     po::value<std::string>(&myCallback)->default_value(""),
     "Callback to receive the return of the function/chain in an asynchronous manner. Mandatory with function chains.")
    ("state-endpoint",
     po::value<std::string>(&myStateEndpoint)->default_value(""),
     "Use the given end-point to get/set the states. If the --no-state-server option is not specified, then a state server is also created listening at this end-point.")
    ("no-state-server", "Do not create a state server. Only makes sense if --state-endpoint is not empty.")
    ("append", "Append to the output file instead of overwriting results.")
    ("dry", "Do not execute the lambda requests, just ask for an estimate of the time required.")
    ("seed",
     po::value<size_t>(&mySeedUser)->default_value(0),
     "Seed generator.")
    ;
  // clang-format on

  try {
    po::variables_map myVarMap;
    po::store(po::parse_command_line(argc, argv, myDesc), myVarMap);
    po::notify(myVarMap);

    if (myVarMap.count("help")) {
      std::cout << myDesc << std::endl;
      return EXIT_FAILURE;
    }

    if (myServerEndpoints.empty()) {
      throw std::runtime_error("Empty end-points: " + myServerEndpoints);
    }

    if (not myChainConf.empty() and not myDagConf.empty()) {
      throw std::runtime_error("Cannot specify both a chain and a DAG");
    }

    if (myStateEndpoint.empty() and myVarMap.count("no-state-server")) {
      throw std::runtime_error(
          "Cannot specify --no-state-server without --state-endpoint");
    }

    std::unique_ptr<ec::model::Chain> myChain;
    std::map<std::string, size_t>     myStateSizes;
    if (not myChainConf.empty()) {
      ec::model::ChainFactory::make(
          uiiit::support::Conf(myChainConf), myChain, myStateSizes);
      if (myChain.get() == nullptr) {
        return EXIT_SUCCESS;
      }
    }

    std::unique_ptr<ec::model::Dag> myDag;
    if (not myDagConf.empty()) {
      ec::model::DagFactory::make(
          uiiit::support::Conf(myDagConf), myDag, myStateSizes);
      if (myDag.get() == nullptr) {
        return EXIT_SUCCESS;
      }
    }

    if (not myCallback.empty() and myNumThreads > 1) {
      throw std::runtime_error(
          "With asynchronous responses it is currently not "
          "possible to have > 1 thread");
    }

    LOG_IF(INFO, myChain.get() != nullptr)
        << "operating in function chain mode, overriding the lambda name: "
        << myLambda;

    LOG_IF(INFO, myDag.get() != nullptr)
        << "operating in function DAG mode, overriding the lambda name: "
        << myLambda;

    assert(myChain.get() == nullptr or myDag.get() == nullptr);

    std::unique_ptr<ResponseSaver> myResponseSaver;
    if (not myCallback.empty()) {
      myResponseSaver = std::make_unique<ResponseSaver>(myCallback);
    }

    const auto myEdgeClientConf = uiiit::support::Conf(myClientConf);

    const auto mySizeSet =
        uiiit::support::split<std::vector<size_t>>(mySizes, ",");
    if (mySizeSet.empty()) {
      throw std::runtime_error("Invalid --sizes option");
    }

    LOG_IF(INFO, not myContent.empty())
        << "Using a custom content for all lambda requests: " << myContent;

    std::unique_ptr<ec::StateServer> myStateServer;
    if (not myStateEndpoint.empty() and
        myVarMap.count("no-state-server") == 0) {
      myStateServer = std::make_unique<ec::StateServer>(myStateEndpoint);
      myStateServer->run(false);
    }

    std::this_thread::sleep_for(
        std::chrono::nanoseconds(static_cast<int64_t>(myInitialDelay * 1e9)));

    const uiiit::support::Saver mySaver(
        myOutputFile, true, myDuration == 0, myVarMap.count("append") == 1);
    using ClientPtr = std::unique_ptr<es::UnifClient>;
    uiiit::support::ThreadPool<ClientPtr> myPool;
    std::list<es::Client*>                myClients;
    for (size_t i = 0; i < myNumThreads; i++) {
      auto myNewClient = ClientPtr(new es::UnifClient(
          mySizeSet,
          myInterRequestTime,
          es::distributionFromString(myInterRequestType),
          mySeedUser,
          i,
          myMaxRequests,
          uiiit::support::split<std::set<std::string>>(myServerEndpoints, ","),
          myEdgeClientConf,
          (myChain.get() == nullptr and myDag.get() == nullptr) ? myLambda : "",
          mySaver,
          myVarMap.count("dry") > 0));
      myNewClient->setContent(myContent);
      if (not myCallback.empty()) {
        myNewClient->setCallback(myCallback);
      }
      if (myChain.get() != nullptr) {
        myNewClient->setChain(*myChain, myStateSizes);
      }
      if (myDag.get() != nullptr) {
        myNewClient->setDag(*myDag, myStateSizes);
      }
      myNewClient->setStateServer(myStateEndpoint); // end-point can be empty
      myClients.push_back(myNewClient.get());
      myPool.add(std::move(myNewClient));
    }

    if (not myCallback.empty()) {
      assert(myNumThreads == 1);
      assert(myClients.size() == 1);
      myResponseSaver->client(myClients.back());
    }

    std::thread myTerminationThread;
    if (myDuration > 0) {
      myTerminationThread = std::thread([&myPool, myDuration]() {
        std::this_thread::sleep_for(std::chrono::seconds(myDuration));
        myPool.stop();
      });
    }

    myPool.start();
    const auto& myErrors = myPool.wait();

    if (myTerminationThread.joinable()) {
      myTerminationThread.join();
    }

    LOG_IF(ERROR, not myErrors.empty())
        << myErrors.size() << " error" << (myErrors.size() > 1 ? "s" : "")
        << " occurred: " << ::toString(myErrors, " | ");

    for (auto myClient : myClients) {
      assert(myClient != nullptr);
      LOG_IF(INFO, myClient->latencyStat().count() >= 1)
          << "latency " << myClient->latencyStat().mean() << " +- "
          << myClient->latencyStat().stddev();
      LOG_IF(INFO, myClient->processingStat().count() >= 1)
          << "processing " << myClient->processingStat().mean() << " +- "
          << myClient->processingStat().stddev();
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Exception caught: " << aErr.what();
  } catch (...) {
    LOG(ERROR) << "Unknown exception caught";
  }

  return EXIT_FAILURE;
}
