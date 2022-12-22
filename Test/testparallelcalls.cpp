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
#include "Support/threadpool.h"
#include "Support/tostring.h"
#include "Support/wait.h"

#include "gtest/gtest.h"

#include <glog/logging.h>

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
      : theEndpoint("127.0.0.1:10000")
      , theNumThreads(5)
      , theComputer(5, theEndpoint, false, [](const auto& aUtil) {})
      , theComputerServerImpl(nullptr) {
    Composer()(
        support::Conf("type=intel-server,num-containers=1,num-workers=1"),
        theComputer.computer());

    theComputerServerImpl.reset(
        new EdgeServerGrpc(theComputer, theNumThreads, false));

    theComputerServerImpl->run();

    LOG(INFO) << "starting test";
  }

  static bool checkResponse(const LambdaResponse& aResp,
                            const size_t          aExpectedSize) {
    return aResp.theResponder == "" and aResp.theRetCode == "OK" and
           aResp.theOutput.size() == aExpectedSize and
           aResp.theDataOut.size() == 0;
  }

  const std::string theEndpoint;
  const size_t      theNumThreads;

  EdgeComputer                    theComputer;
  std::unique_ptr<EdgeServerImpl> theComputerServerImpl;

  const size_t N        = 10;
  const size_t NTHREADS = 2;
};

TEST_F(TestParallelCalls, DISABLED_test_single) {
  EdgeClientGrpc myClient(theEndpoint, false);

  using Data = std::map<size_t, std::vector<double>>;
  const std::list<size_t> mySizes({10, 1000});
  Data                    myDelaysSingle;
  support::Chrono         myChrono(false);

  // run one call at a time
  for (const auto& mySize : mySizes) {
    const auto it = myDelaysSingle.emplace(mySize, std::vector<double>(N, 0));
    LambdaRequest myReq("clambda0", std::string(mySize, 'A'));
    for (size_t i = 0; i < N; i++) {
      myChrono.start();
      ASSERT_TRUE(checkResponse(myClient.RunLambda(myReq, false), mySize));
      it.first->second.at(i) = myChrono.stop();
    }
  }

  // run calls in parallel
  struct RandomCaller {
    RandomCaller(EdgeClientGrpc&          aClient,
                 const std::list<size_t>& aSizes,
                 const size_t             aIterations,
                 Data&                    aData)
        : theClient(aClient)
        , theSizes(aSizes)
        , theIterations(aIterations)
        , theData(aData) {
      // noop
    }

    void operator()() {
      support::UniformRv myRv(0, 1, ::time(nullptr), 0, 0);
      support::Chrono    myChrono(false);
      for (size_t i = 0; i < theIterations; i++) {
        const auto    mySize = support::choice(theSizes, myRv);
        LambdaRequest myReq("clambda0", std::string(mySize, 'A'));
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
  };
  support::ThreadPool<RandomCaller> myThreadPool;
  Data                              myDelaysDouble;
  for (size_t i = 0; i < NTHREADS; i++) {
    myThreadPool.add(RandomCaller(myClient, mySizes, N, myDelaysDouble));
  }
  myThreadPool.start();
  ASSERT_TRUE(myThreadPool.wait().empty());

  // print delays
  std::list<Data*> myDataPtrs({&myDelaysSingle, &myDelaysDouble});
  for (const auto myDataPtr : myDataPtrs) {
    for (const auto& elem : *myDataPtr) {
      std::cout << elem.first << '\t' << ::toStringStd(elem.second, "\t")
                << std::endl;
    }
  }
}

} // namespace edge
} // namespace uiiit
