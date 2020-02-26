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

#include "Edge/edgeclientpool.h"
#include "Support/chrono.h"
#include "Support/glograii.h"
#include "Support/queue.h"
#include "Support/saver.h"
#include "Support/stat.h"

#include <glog/logging.h>

#include <boost/program_options.hpp>

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <thread>

namespace po = boost::program_options;
namespace ec = uiiit::edge;

struct Event {
  float       theTime;
  std::string theLambda;
  std::string theEdgeServer;
  size_t      theSize;
};

void readFromFile(const std::string&            aInputFile,
                  uiiit::support::Queue<Event>& aEvents) {
  std::ifstream myInFile(aInputFile);
  if (not myInFile) {
    return;
  }
  while (true) {
    Event myEvent;
    myInFile >> myEvent.theTime >> myEvent.theLambda >> myEvent.theEdgeServer >>
        myEvent.theSize;
    if (not myInFile.good()) {
      break;
    }
    aEvents.push(std::move(myEvent));
  }
}

struct Consumer {
  Consumer(uiiit::support::Queue<Event>& aEvents,
           std::atomic<size_t>&          aDispatched,
           std::atomic<size_t>&          aDeadlineMisses,
           uiiit::support::Chrono&       aChrono,
           const uiiit::support::Saver&  aSaver,
           uiiit::support::SummaryStat&  aStat,
           ec::EdgeClientPool&           aClientPool)
      : theEvents(aEvents)
      , theDispatched(aDispatched)
      , theDeadlineMisses(aDeadlineMisses)
      , theChrono(aChrono)
      , theSaver(aSaver)
      , theStat(aStat)
      , theClientPool(aClientPool) {
  }

  void consume() {
    const auto myEvent = theEvents.pop(); // blocking

    // sleep until it is time to proceed
    const auto mySleepTime      = myEvent.theTime - theChrono.time();
    auto       myDeadlineMissed = false;
    if (mySleepTime <= 0) {
      myDeadlineMissed = true;
    } else {
      std::this_thread::sleep_for(
          std::chrono::microseconds(static_cast<int64_t>(mySleepTime * 1e6)));
    }
    theDeadlineMisses += myDeadlineMissed ? 1 : 0;

    // fire the request, wait for the response

    ec::LambdaRequest myReq(myEvent.theLambda,
                            std::string(myEvent.theSize, 'A'));

    const auto ret = theClientPool(myEvent.theEdgeServer, myReq, false);

    if (ret.first.theRetCode == "OK") {
      theSaver(ret.second, ret.first.theLoad1, ret.first.theResponder, false);
      theStat(ret.second);
      VLOG(1) << "at " << myEvent.theTime << " s, " << myEvent.theLambda
              << ", to " << myEvent.theEdgeServer << ", took "
              << (myEvent.theTime * 1e3 + 0.5) << " ms, deadline "
              << (myDeadlineMissed ? "miss" : "hit") << ", " << ret.first;
    } else {
      LOG(ERROR) << "invalid response received from " << myEvent.theEdgeServer
                 << ": " << ret.first.theRetCode;
    }

    ++theDispatched;
  }

  uiiit::support::Queue<Event>& theEvents;
  std::atomic<size_t>&          theDispatched;
  std::atomic<size_t>&          theDeadlineMisses;
  uiiit::support::Chrono&       theChrono;
  const uiiit::support::Saver&  theSaver;
  uiiit::support::SummaryStat&  theStat;
  ec::EdgeClientPool&           theClientPool;
};

int main(int argc, char* argv[]) {
  uiiit::support::GlogRaii myGlogRaii(argv[0]);

  std::string myInputFile;
  std::string myOutputFile;
  size_t      myNumThreads;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
    ("help,h", "produce help message")
    ("num-threads",
     po::value<size_t>(&myNumThreads)->default_value(5),
     "Number of threads.")
    ("input-file",
     po::value<std::string>(&myInputFile)->default_value("/dev/stdin"),
     "Input file name.")
    ("output-file",
     po::value<std::string>(&myOutputFile)->default_value(""),
     "Output file name. Do not save latencies if empty.")
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

    uiiit::support::Queue<Event> myEvents;
    readFromFile(myInputFile, myEvents);
    const auto myTotalEvents = myEvents.size();

    LOG(INFO) << "read " << myTotalEvents << " events from " << myInputFile;

    std::atomic<size_t> myDispatched{0};
    std::atomic<size_t> myDeadlineMisses{0};
    std::atomic<bool>   myError{false};

    uiiit::support::Chrono      myChrono(true);
    const uiiit::support::Saver mySaver(myOutputFile, true, false, false);
    uiiit::support::SummaryStat myStat;
    ec::EdgeClientPool          myClientPool(0); // unlimited clients

    std::list<Consumer>    myConsumers;
    std::list<std::thread> myThreads;
    for (size_t i = 0; i < myNumThreads; i++) {
      myConsumers.emplace_back(Consumer(myEvents,
                                        myDispatched,
                                        myDeadlineMisses,
                                        myChrono,
                                        mySaver,
                                        myStat,
                                        myClientPool));
      auto& myConsumer = myConsumers.back();
      myThreads.emplace_back(std::thread([&myConsumer, &myError]() {
        while (true) {
          try {
            myConsumer.consume();
          } catch (const uiiit::support::QueueClosed&) {
            VLOG(2) << "done";
            break;
          } catch (const std::runtime_error& aErr) {
            myError = true;
            LOG(ERROR) << "Exception found: " << aErr.what();
          } catch (...) {
            myError = true;
            LOG(ERROR) << "Unknown exception found";
          }
        }
      }));
    }

    while (myDispatched.load() < myTotalEvents and not myError.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LOG_IF(ERROR, myError.load()) << "An error occurred";
    myEvents.close();
    for (auto& myThread : myThreads) {
      myThread.join();
    }

    LOG(INFO) << "dispatched      " << myDispatched.load() << " / "
              << myTotalEvents;
    LOG(INFO) << "deadline misses " << myDeadlineMisses.load();
    LOG(INFO) << "average latency " << myStat.mean();

    return EXIT_SUCCESS;

  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Exception caught: " << aErr.what();

  } catch (...) {
    LOG(ERROR) << "Unknown exception caught";
  }

  return EXIT_FAILURE;
}
