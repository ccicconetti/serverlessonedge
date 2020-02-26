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

#include "Edge/edgecomputerserver.h"
#include "Edge/edgecontrollerclient.h"
#include "Edge/edgecontrollermessages.h"
#include "Edge/processloadserver.h"
#include "OpenCV/facedetectcomputer.h"
#include "Support/checkfiles.h"
#include "Support/conf.h"
#include "Support/glograii.h"
#include "Support/signalhandlerwait.h"
#include "Support/system.h"

#include <glog/logging.h>

#include <boost/program_options.hpp>

#include <array>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

namespace po = boost::program_options;
namespace ec = uiiit::edge;

int main(int argc, char* argv[]) {
  uiiit::support::GlogRaii          myGlogRaii(argv[0]);
  uiiit::support::SignalHandlerWait mySignalHandler;

  std::string myServerEndpoint;
  std::string myUtilServerEndpoint;
  std::string myControllerEndpoint;
  size_t      myNumThreads;
  size_t      myNumWorkers;
  std::string myModels;
  std::string myDummyLambda;
  double      myScale;

  po::options_description myDesc("Allowed options");

  // clang-format off
  myDesc.add_options()
  ("help,h", "produce help message")
  ("server-endpoint",
   po::value<std::string>(&myServerEndpoint)->default_value("0.0.0.0:6473"),
   "Server end-point.")
  ("utilization-endpoint",
   po::value<std::string>(&myUtilServerEndpoint)->default_value("0.0.0.0:6476"),
   "Utilization server end-point. If empty utilization is not computed.")
  ("controller-endpoint",
   po::value<std::string>(&myControllerEndpoint)->default_value(""),
   "If specified announce to this controller.")
  ("num-threads",
   po::value<size_t>(&myNumThreads)->default_value(4),
   "Number of threads that can be used by the OpenCV library.")
  ("num-workers",
   po::value<size_t>(&myNumWorkers)->default_value(1),
   "Number of workers.")
  ("scale",
   po::value<double>(&myScale)->default_value(1.0),
   "Image scale.")
  ("dummy-lambda",
   po::value<std::string>(&myDummyLambda)->default_value("clambda0"),
   "Name of a lambda function that returns immediately with success.")
  ("models",
   po::value<std::string>(&myModels)->default_value(
     "facerec0=haarcascade_frontalface_default.xml,"
     "eyerec0=haarcascade_eye_tree_eyeglasses.xml"),
   "List of comma-separated pairs lambda_name=model_file.")
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

    if (myServerEndpoint.empty()) {
      throw std::runtime_error("Empty end-point: " + myServerEndpoint);
    }

    const uiiit::support::Conf myConf(myModels);
    if (myConf.empty()) {
      throw std::runtime_error("Empty list of models");
    }

    // check that the object detection model files exist and are regular
    uiiit::support::checkFiles(myConf.values());

    // warn if the user asked for more parallel jobs than processors
    LOG_IF(WARNING, myNumThreads > std::thread::hardware_concurrency())
        << myNumThreads << " threads requested with only "
        << std::thread::hardware_concurrency()
        << " processors: performance may now scale as expected";

    // start the background process providing the user/system loads via gRPC
    std::unique_ptr<ec::ProcessLoadServer>     myProcessLoadServer;
    std::function<std::array<double, 3>(void)> myProcessLoadCallback = []() {
      return std::array<double, 3>{{0.0, 0.0, 0.0}};
    };
    if (myUtilServerEndpoint.empty()) {
      VLOG(1) << "No utilization server specified: load notifications disabled";

    } else {
      myProcessLoadServer.reset(new ec::ProcessLoadServer(
          myUtilServerEndpoint,
          uiiit::support::System::instance().cpuName(),
          1.0));
      myProcessLoadCallback = [&myProcessLoadServer]() {
        return myProcessLoadServer->lastUtils();
      };
    }

    // start the server
    ec::FaceDetectComputer myServer(myServerEndpoint,
                                    myNumWorkers,
                                    myNumThreads,
                                    myConf,
                                    myScale,
                                    myDummyLambda,
                                    myProcessLoadCallback);

    // announce the edge computer to the edge controller, if known
    if (myControllerEndpoint.empty()) {
      VLOG(1) << "No controller specified: announce disabled";

    } else {
      ec::EdgeControllerClient myControllerClient(myControllerEndpoint);
      ec::ContainerList        myContainerList;
      for (const auto& myLambda : myConf.keys()) {
        myContainerList.theContainers.emplace_back(ec::ContainerList::Container{
            uiiit::support::System::instance().hostName() + "_" + myLambda,
            "unknown_cpu",
            myLambda,
            static_cast<uint32_t>(myNumThreads)});
      }
      myContainerList.theContainers.emplace_back(ec::ContainerList::Container{
          myDummyLambda,
          uiiit::support::System::instance().cpuName(),
          myDummyLambda,
          1});
      myControllerClient.announceComputer(myServerEndpoint, myContainerList);
      LOG(INFO) << "Announced to " << myControllerEndpoint;
    }

    // waiting until terminated by the user
    myServer.run(); // non-blocking
    // myServer.wait();     // blocking
    mySignalHandler.wait(); // blocking

    // perform clean exit by removing this computer from the controller
    if (not myControllerEndpoint.empty()) {
      ec::EdgeControllerClient myControllerClient(myControllerEndpoint);
      myControllerClient.removeComputer(myServerEndpoint);
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Exception caught: " << aErr.what();
  } catch (...) {
    LOG(ERROR) << "Unknown exception caught";
  }

  return EXIT_FAILURE;
}
