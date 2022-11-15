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

#include "LambdaMuSim/simulation.h"
#include "Support/chrono.h"
#include "Support/glograii.h"
#include "Support/versionutils.h"

#include <boost/program_options.hpp>

#include <glog/logging.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace po = boost::program_options;
namespace ls = uiiit::lambdamusim;
namespace us = uiiit::support;

bool explainOrPrint(const po::variables_map& aVarMap) {
  if (aVarMap.count("explain-output") == 1 and
      aVarMap.count("print-header") == 1) {
    throw std::runtime_error(
        "cannot specify both --explain-output and --print-header");
  }
  if (aVarMap.count("explain-output") == 1) {
    std::size_t myCol = 0;
    std::cout << '#' << ++myCol << "\t[in]  seed\n";
    for (const auto& elem : ls::Conf::toColumns()) {
      std::cout << '#' << ++myCol << "\t[in]  " << elem << '\n';
    }
    for (const auto& elem : ls::PerformanceData::toColumns()) {
      std::cout << '#' << ++myCol << "\t[out] " << elem << '\n';
    }
    return true;
  }

  if (aVarMap.count("print-header") == 1) {
    std::cout << "seed";
    for (const auto& elem : ls::Conf::toColumns()) {
      std::cout << ',' << elem;
    }
    for (const auto& elem : ls::PerformanceData::toColumns()) {
      std::cout << ',' << elem;
    }
    std::cout << "\n";
    return true;
  }
  return false;
}

int main(int argc, char* argv[]) {
  us::GlogRaii myGlogRaii(argv[0]);

  std::string myTypeStr;
  std::string myNodesPath;
  std::string myLinksPath;
  std::string myEdgesPath;
  double      myCloudDistanceFactor;
  double      myCloudStorageCostLocal;
  double      myCloudStorageCostRemote;
  std::string myAppsPath;
  double      myDuration;
  double      myWarmUp;
  double      myEpoch;
  std::size_t myMinPeriods;
  std::size_t myAvgApps;
  std::size_t myAvgLambda;
  std::size_t myAvgMu;
  double      myAlpha;
  double      myBeta;
  std::string myAppModel;
  std::string myOutfile;

  std::size_t myStartingSeed;
  std::size_t myNumReplications;
  std::size_t myNumThreads;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
    ("help,h",         "produce help message")
    ("version,v",      "print the version and quit")
    ("explain-output", "report the meaning of the columns in the output")
    ("print-header",   "print the header of the CSV output file")

    ("type",
     po::value<std::string>(&myTypeStr)->default_value("snapshot"),
     "Simulation type, one of: {snapshot, dynamic}.")
    ("nodes-path",
     po::value<std::string>(&myNodesPath)->default_value("nodes"),
     "File containing the specifications of nodes.")
    ("links-path",
     po::value<std::string>(&myLinksPath)->default_value("links"),
     "File containing the specifications of links.")
    ("edges-path",
     po::value<std::string>(&myEdgesPath)->default_value("edges"),
     "File containing the specifications of edges.")
    ("cloud-distance-factor",
     po::value<double>(&myCloudDistanceFactor)->default_value(2.0),
     "The cloud distance scale factor.")
    ("cloud-storage-cost-local",
     po::value<double>(&myCloudStorageCostLocal)->default_value(0.0),
     "The cost to access cloud storage from the cloud, per data unit (lambda apps only).")
    ("cloud-storage-cost-remote",
     po::value<double>(&myCloudStorageCostRemote)->default_value(0.0),
     "The cost to access cloud storage from the edge, per data unit (lambda apps only).")
    ("apps-path",
     po::value<std::string>(&myAppsPath)->default_value("apps"),
     "File containing the specifications of applications (dynamic only).")
    ("duration",
     po::value<double>(&myDuration)->default_value(300000),
     "Simulation duration (dynamic only)")
    ("warm-up",
     po::value<double>(&myWarmUp)->default_value(5000),
     "Warm-up duration (dynamic only)")
    ("epoch",
     po::value<double>(&myEpoch)->default_value(60000),
     "Epoch duration (dynamic only)")
    ("min-periods",
     po::value<std::size_t>(&myMinPeriods)->default_value(50),
     "Only use apps from dataset with at least these periods (dynamic only)")
    ("avg-apps",
     po::value<std::size_t>(&myAvgApps)->default_value(10),
     "Average number of apps (dynamic only)")
    ("avg-lambda",
     po::value<std::size_t>(&myAvgLambda)->default_value(10),
     "The average number of lamba-apps (snapshot only).")
    ("avg-mu",
     po::value<std::size_t>(&myAvgMu)->default_value(10),
     "The average number of mu-apps (snapshot only).")
    ("alpha",
     po::value<double>(&myAlpha)->default_value(0.5),
     "The lambda-app reservation factor.")
    ("beta",
     po::value<double>(&myBeta)->default_value(0.5),
     "The lambda-app overprovisioning factor.")
    ("app-model",
     po::value<std::string>(&myAppModel)->default_value("constant,1,1,1"),
     "The model that defines the applications' parameters.")
    ("out-file",
     po::value<std::string>(&myOutfile)->default_value("out.csv"),
     "The file where to save the results.")
    ("append", "Append the results to the output.")
    ("seed-starting",
     po::value<size_t>(&myStartingSeed)->default_value(1),
     "The starting seed.")
    ("num-replications",
     po::value<size_t>(&myNumReplications)->default_value(1),
     "The number of replications.")
    ("num-threads",
     po::value<size_t>(&myNumThreads)->default_value(
       std::max(1u,
                std::thread::hardware_concurrency())),
     "The number of threads to spawn.")
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

    if (myVarMap.count("version")) {
      std::cout << us::version() << std::endl;
      return EXIT_SUCCESS;
    }

    if (explainOrPrint(myVarMap)) {
      return EXIT_SUCCESS;
    }

    ls::Conf::Type myType;
    if (myTypeStr == "snapshot") {
      myType = ls::Conf::Type::Snapshot;
    } else if (myTypeStr == "dynamic") {
      myType = ls::Conf::Type::Dynamic;
    } else {
      throw std::runtime_error("Invalid simulation type: " + myTypeStr);
    }

    us::Chrono myChrono(true);
    ls::Simulation(myNumThreads)
        .run(ls::Conf{myType,
                      myNodesPath,
                      myLinksPath,
                      myEdgesPath,
                      myCloudDistanceFactor,
                      myCloudStorageCostLocal,
                      myCloudStorageCostRemote,
                      myAppsPath,
                      myDuration,
                      myWarmUp,
                      myEpoch,
                      myMinPeriods,
                      myAvgApps,
                      myAvgLambda,
                      myAvgMu,
                      myAlpha,
                      myBeta,
                      myAppModel,
                      myOutfile,
                      myVarMap.count("append") == 1},
             myStartingSeed,
             myNumReplications);

    LOG(INFO) << "simulation lasted " << myChrono.stop() << " s";

    return EXIT_SUCCESS;

  } catch (const std::exception& aErr) {
    std::cerr << "Exception caught: " << aErr.what() << std::endl;

  } catch (...) {
    std::cerr << "Unknown exception caught" << std::endl;
  }

  return EXIT_FAILURE;
}
