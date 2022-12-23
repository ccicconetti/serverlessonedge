/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2022 C. Cicconetti <https://ccicconetti.github.io/>

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

#include "Edge/composer.h"
#include "Edge/computer.h"
#include "Edge/edgeclientgrpc.h"
#include "Edge/edgecomputer.h"
#include "Edge/edgeservergrpc.h"
#include "Edge/edgeserverimpl.h"
#include "Support/chrono.h"
#include "Support/conf.h"
#include "Support/random.h"
#include "Support/split.h"
#include "Support/threadpool.h"
#include "Support/tostring.h"
#include "Support/wait.h"

#include "gtest/gtest.h"

#include <fstream>
#include <glog/logging.h>

#include <cstdlib>
#include <ctime>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace uiiit {
namespace edge {

struct TestParallelCalls : public ::testing::Test {
  TestParallelCalls()
      : theEndpoint("localhost:10000")
      , theOutput("out")
      , theLambda("clambda0")
      , theSizes({10, 100})
      , theComputer(nullptr)
      , theComputerServerImpl(nullptr) {
    getEnv();

    if (theEndpoint.find("localhost") != std::string::npos) {
      theComputer = std::make_unique<EdgeComputer>(
          theConcurrency, theEndpoint, theSecure, [](const auto&) {});

      Composer()(
          support::Conf(std::string("type=intel-server,num-containers=") +
                        std::to_string(theConcurrency) +
                        ",num-workers=" + std::to_string(theConcurrency)),
          theComputer->computer());

      theComputerServerImpl.reset(
          new EdgeServerGrpc(*theComputer, theConcurrency, theSecure));

      theComputerServerImpl->run();
    }
  }

  void getEnv() {
    const auto myEnvEndpoint = ::getenv("ENDPOINT");
    if (myEnvEndpoint != nullptr) {
      theEndpoint = std::string(myEnvEndpoint);
    }
    const auto myEnvConcurrency = ::getenv("CONCURRENCY");
    if (myEnvConcurrency != nullptr) {
      theConcurrency = std::stoull(std::string(myEnvConcurrency));
    }
    const auto myEnvNumCalls = ::getenv("NUMCALLS");
    if (myEnvNumCalls != nullptr) {
      theNumCalls = std::stoull(std::string(myEnvNumCalls));
    }
    const auto myEnvOutput = ::getenv("OUTPUT");
    if (myEnvOutput != nullptr) {
      theOutput = std::string(myEnvOutput);
    }
    const auto myEnvLambda = ::getenv("LAMBDA");
    if (myEnvLambda != nullptr) {
      theLambda = std::string(myEnvLambda);
    }
    const auto myEnvSizes = ::getenv("SIZES");
    if (myEnvSizes != nullptr) {
      theSizes =
          support::split<std::list<size_t>>(std::string(myEnvSizes), ",");
    }
    const auto myEnvSecure = ::getenv("SECURE");
    if (myEnvSecure != nullptr) {
      theSecure = std::string(myEnvSecure) != "0";
    }
  }

  static bool checkResponse(const LambdaResponse& aResp,
                            const size_t          aExpectedSize) {
    return aResp.theResponder == "" and aResp.theRetCode == "OK" and
           aResp.theOutput.size() == aExpectedSize and
           aResp.theDataOut.size() == 0;
  }

  // can be overridden by environment variables
  std::string       theEndpoint;
  size_t            theConcurrency = 2;
  size_t            theNumCalls    = 10;
  std::string       theOutput;
  std::string       theLambda;
  std::list<size_t> theSizes;
  bool              theSecure = false;

  std::unique_ptr<EdgeComputer>   theComputer;
  std::unique_ptr<EdgeServerImpl> theComputerServerImpl;
};

TEST_F(TestParallelCalls, DISABLED_test_single) {

  EdgeClientGrpc myClient(theEndpoint, theSecure);

  using Data = std::map<size_t, std::vector<double>>;
  Data myDelays;

  // run calls in parallel
  struct RandomCaller {
    RandomCaller(EdgeClientGrpc&          aClient,
                 const std::list<size_t>& aSizes,
                 const size_t             aIterations,
                 Data&                    aData,
                 const std::string&       aLambda)
        : theClient(aClient)
        , theSizes(aSizes)
        , theIterations(aIterations)
        , theData(aData)
        , theLambda(aLambda) {
      // noop
    }

    void operator()() {
      support::UniformRv myRv(0, 1, ::time(nullptr), 0, 0);
      support::Chrono    myChrono(false);
      for (size_t i = 0; i < theIterations; i++) {
        const auto    mySize = support::choice(theSizes, myRv);
        LambdaRequest myReq(theLambda, std::string(mySize, 'A'));
        myChrono.start();
        const auto mySuccess =
            checkResponse(theClient.RunLambda(myReq, false), mySize);
        const auto myElapsed = myChrono.stop();
        theData[mySize].emplace_back(mySuccess ? myElapsed : -1);
      }
    }

    EdgeClientGrpc&         theClient;
    const std::list<size_t> theSizes;
    const size_t            theIterations;
    Data&                   theData;
    const std::string       theLambda;
  };
  support::ThreadPool<RandomCaller> myThreadPool;
  for (size_t i = 0; i < theConcurrency; i++) {
    myThreadPool.add(
        RandomCaller(myClient, theSizes, theNumCalls, myDelays, theLambda));
  }
  myThreadPool.start();
  const auto ret = myThreadPool.wait();
  ASSERT_TRUE(ret.empty()) << '\n' << ::toString(ret, "\n");

  // save delays
  for (const auto& elem : myDelays) {
    const auto myFilename =
        theOutput + "-" + std::to_string(elem.first) + ".dat";
    std::ofstream myOut(myFilename);
    ASSERT_TRUE(myOut) << "could not open file for writing: " << myFilename;
    for (const auto& myValue : elem.second) {
      myOut << myValue << '\n';
    }
  }
}

} // namespace edge
} // namespace uiiit
