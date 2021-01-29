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

#include "Edge/edgeclientgrpc.h"
#include "Simulation/ippclient.h"
#include "Simulation/poissonclient.h"
#include "Support/chrono.h"
#include "Support/glograii.h"
#include "Support/macros.h"
#include "Support/saver.h"
#include "Support/split.h"
#include "Support/threadpool.h"

#include <glog/logging.h>

#include <boost/program_options.hpp>

#include <cstdlib>
#include <fstream>
#include <random>
#include <thread>

namespace po = boost::program_options;
namespace ec = uiiit::edge;
namespace es = uiiit::simulation;

int main(int argc, char* argv[]) {
  uiiit::support::GlogRaii myGlogRaii(argv[0]);

  std::string myServerEndpoints;
  std::string myClientConf;
  std::string myLambda;
  std::string myClientType;
  size_t      myDuration;
  size_t      myMaxRequests;
  size_t      myNumClients;
  size_t      mySizeMin;
  size_t      mySizeMax;
  std::string mySizes;
  double      myOnMean;
  double      myOffMean;
  double      myPeriodMean;
  double      myBurstSizeMean;
  double      mySleepMin;
  double      mySleepMax;
  std::string myContent;
  std::string myOutfile;
  std::string myLossfile;
  size_t      mySeedUser;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
    ("help,h", "produce help message")
    ("server-endpoints",
     po::value<std::string>(&myServerEndpoints)->default_value("localhost:6473"),
     "Server end-points (comma-separated list).")
    ("client-conf",
     po::value<std::string>(&myClientConf)->default_value("type=grpc,persistence=0.05"),
     "Configuration of the edge clients.")
    ("lambda",
     po::value<std::string>(&myLambda)->default_value("clambda0"),
     "Lambda function name.")
    ("client-type",
     po::value<std::string>(&myClientType)->default_value("poisson"),
     "Client type, one of: ipp, poisson.")
    ("duration",
     po::value<size_t>(&myDuration)->default_value(0),
     "Experiment duration, in s. 0 means infinite.")
    ("max-requests",
     po::value<size_t>(&myMaxRequests)->default_value(std::numeric_limits<size_t>::max()),
     "Number of requests to issue.")
    ("num-clients",
     po::value<size_t>(&myNumClients)->default_value(1),
     "Number of clients.")
    ("min-size",
     po::value<size_t>(&mySizeMin)->default_value(100),
     "Minimum request size, in bytes. Ignored if --sizes is used.")
    ("max-size",
     po::value<size_t>(&mySizeMax)->default_value(100),
     "Maximum request size, in bytes. Ignored if --sizes is used.")
    ("sizes",
     po::value<std::string>(&mySizes)->default_value(""),
     "Input sizes, separated by comma. Repeated values admitted.")
    ("on-mean",
     po::value<double>(&myOnMean)->default_value(1),
     "Average duration of the ON period, in s. Only meaningful with ipp clients.")
    ("off-mean",
     po::value<double>(&myOffMean)->default_value(1),
     "Average duration of the OFF period, in s. Only meaningful with ipp clients.")
    ("period-mean",
     po::value<double>(&myPeriodMean)->default_value(5),
     "Average duration of the period, in s. Only meaningful with poisson clients.")
    ("burst-size-mean",
     po::value<double>(&myBurstSizeMean)->default_value(5),
     "Average duration of the burst size, in lambda requests. "
     "Only meaningful with poisson clients.")
    ("min-sleep",
     po::value<double>(&mySleepMin)->default_value(0),
     "Minimum sleep duration after a response in an ON period, in s.")
    ("max-sleep",
     po::value<double>(&mySleepMax)->default_value(0.1),
     "Maximum sleep duration after a response in an ON period, in s."
     "Must be greater than min-sleep.")
    ("content",
     po::value<std::string>(&myContent)->default_value(""),
     "Use the given fixed content for all lambda requests.")
    ("output-file",
     po::value<std::string>(&myOutfile)->default_value("/dev/stdout"),
     "Output filename.")
    ("loss-file",
     po::value<std::string>(&myLossfile)->default_value("/dev/stdout"),
     "Loss filename.")
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

    static const std::set<std::string> myClientTypes({"ipp", "poisson"});
    if (myClientTypes.count(myClientType) == 0) {
      std::cout << "Invalid client type " << myClientType
                << ", choose one of: ";
      for (const auto& myElem : myClientTypes) {
        std::cout << myElem;
      }
      std::cout << std::endl;
      return EXIT_FAILURE;
    }

    const auto mySizeSet =
        uiiit::support::split<std::vector<size_t>>(mySizes, ",");
    LOG_IF(INFO, not mySizeSet.empty())
        << "ignoring --min-size and --max-size values";

    const uiiit::support::Saver mySaver(
        myOutfile, true, myDuration == 0, false);
    using ClientPtr = std::unique_ptr<es::Client>;
    uiiit::support::ThreadPool<ClientPtr> myPool;
    std::list<es::Client*>                myClients;
    size_t                                myNumRequestsExpected = 0;

    for (size_t i = 0; i < myNumClients; i++) {
      ClientPtr myClient(nullptr);
      if (myClientType == "ipp") {
        myClient.reset(
            mySizes.empty() ?
                new es::IppClient(myOnMean,
                                  myOffMean,
                                  mySleepMin,
                                  mySleepMax,
                                  mySizeMin,
                                  mySizeMax,
                                  mySeedUser,
                                  i,
                                  myMaxRequests,
                                  uiiit::support::split<std::set<std::string>>(
                                      myServerEndpoints, ","),
                                  uiiit::support::Conf(myClientConf),
                                  myLambda,
                                  mySaver,
                                  false) :
                new es::IppClient(myOnMean,
                                  myOffMean,
                                  mySleepMin,
                                  mySleepMax,
                                  mySizeSet,
                                  mySeedUser,
                                  i,
                                  myMaxRequests,
                                  uiiit::support::split<std::set<std::string>>(
                                      myServerEndpoints, ","),
                                  uiiit::support::Conf(myClientConf),
                                  myLambda,
                                  mySaver,
                                  false));
        if (myDuration > 0) {
          myNumRequestsExpected += myClient->simulate(myDuration);
        }
      } else if (myClientType == "poisson") {
        myClient.reset(mySizes.empty() ?
                           new es::PoissonClient(
                               myPeriodMean,
                               myBurstSizeMean,
                               mySleepMin,
                               mySleepMax,
                               mySizeMin,
                               mySizeMax,
                               mySeedUser,
                               i,
                               myMaxRequests,
                               uiiit::support::split<std::set<std::string>>(
                                   myServerEndpoints, ","),
                               uiiit::support::Conf(myClientConf),
                               myLambda,
                               mySaver,
                               false) :
                           new es::PoissonClient(
                               myPeriodMean,
                               myBurstSizeMean,
                               mySleepMin,
                               mySleepMax,
                               mySizeSet,
                               mySeedUser,
                               i,
                               myMaxRequests,
                               uiiit::support::split<std::set<std::string>>(
                                   myServerEndpoints, ","),
                               uiiit::support::Conf(myClientConf),
                               myLambda,
                               mySaver,
                               false));
      }
      assert(myClient != nullptr);
      myClient->setContent(myContent);
      myClients.push_back(myClient.get());
      myPool.add(std::move(myClient));
    }

    std::thread myTerminationThread;
    if (myDuration > 0) {
      myTerminationThread = std::thread([&myPool, myDuration]() {
        std::this_thread::sleep_for(std::chrono::seconds(myDuration));
        myPool.stop();
      });
    }

    LOG_IF(INFO, myNumRequestsExpected > 0)
        << "Expecting up to " << myNumRequestsExpected << " lambda requests";

    myPool.start();

    const auto& myErrors = myPool.wait();
    if (myTerminationThread.joinable()) {
      myTerminationThread.join();
    }

    std::stringstream myErrorStr;
    for (const auto& myError : myErrors) {
      myErrorStr << '\n' << myError;
    }
    LOG_IF(ERROR, not myErrors.empty())
        << "Errors occurred:" << myErrorStr.str();

    size_t myRequestsDone = 0;
    for (auto myClient : myClients) {
      assert(myClient != nullptr);
      LOG_IF(INFO, myClient->latencyStat().count() >= 1)
          << "latency " << myClient->latencyStat().mean() << " +- "
          << myClient->latencyStat().stddev();
      LOG_IF(INFO, myClient->processingStat().count() >= 1)
          << "processing " << myClient->processingStat().mean() << " +- "
          << myClient->processingStat().stddev();
      myRequestsDone += myClient->latencyStat().count();
    }

    // save number of lost opportunities to send a lambda request
    if (myNumRequestsExpected > 0) {
      const uiiit::support::Saver myLossSaver(myLossfile, false, true, false);
      myLossSaver(std::max(0.0,
                           1.0 - static_cast<double>(myRequestsDone) /
                                     myNumRequestsExpected));
    }

    return EXIT_SUCCESS;

  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Exception caught: " << aErr.what();

  } catch (...) {
    LOG(ERROR) << "Unknown exception caught";
  }

  return EXIT_FAILURE;
}
