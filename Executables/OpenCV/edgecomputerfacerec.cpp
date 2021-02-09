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

#include "Edge/edgecomputerserver.h"
#include "Edge/edgecontrollerclient.h"
#include "Edge/edgecontrollermessages.h"
#include "Edge/edgeserverimpl.h"
#include "Edge/edgeserverimplfactory.h"
#include "Edge/edgeserveroptions.h"
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

  std::string myServerConf;
  std::string myUtilServerEndpoint;
  std::string myModels;
  std::string myDummyLambda;
  double      myScale;
  size_t      myNumThreads;

  po::options_description myDesc("Allowed options");

  // clang-format off
  myDesc.add_options()
  ("help,h", "produce help message")
  ("server-conf",
   po::value<std::string>(&myServerConf)->default_value("type=grpc"),
   "Comma-separated configuration of the server.") // endpoint, numthreads OR string of conf parameters to build HQParams
  ("utilization-endpoint",
   po::value<std::string>(&myUtilServerEndpoint)->default_value("0.0.0.0:6476"),
   "Utilization server end-point. If empty utilization is not computed.")
  ("num-threads",
   po::value<size_t>(&myNumThreads)->default_value(4),
   "Number of threads that can be used by the OpenCV library.")
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
    uiiit::edge::EdgeServerOptions myCli(argc, argv, myDesc);

    if (myCli.varMap().count("help")) {
      std::cout << myDesc << std::endl;
      return EXIT_FAILURE;
    }

    if (myCli.serverEndpoint().empty()) {
      throw std::runtime_error("Empty end-point: " + myCli.serverEndpoint());
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

    // create server
    ec::FaceDetectComputer myServer(myCli.serverEndpoint(),
                                    myNumThreads,
                                    myConf,
                                    myScale,
                                    myDummyLambda,
                                    myProcessLoadCallback);

    // create transport layer
    const auto myServerImpl =
        ec::EdgeServerImplFactory::make(myServer,
                                        myCli.serverEndpoint(),
                                        myCli.numThreads(),
                                        uiiit::support::Conf(myServerConf));

    // announce the edge computer to the edge controller, if known
    if (myCli.controllerEndpoint().empty()) {
      VLOG(1) << "No controller specified: announce disabled";

    } else {
      ec::EdgeControllerClient myControllerClient(myCli.controllerEndpoint());
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
      myControllerClient.announceComputer(myCli.serverEndpoint(),
                                          myContainerList);
      LOG(INFO) << "Announced to " << myCli.controllerEndpoint();
    }

    // waiting until terminated by the user
    myServerImpl->run(); // non-blocking
    // myServerImpl->wait(); // blocking
    mySignalHandler.wait(); // blocking

    // perform clean exit by removing this computer from the controller
    if (not myCli.controllerEndpoint().empty()) {
      ec::EdgeControllerClient myControllerClient(myCli.controllerEndpoint());
      myControllerClient.removeComputer(myCli.serverEndpoint());
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Exception caught: " << aErr.what();
  } catch (...) {
    LOG(ERROR) << "Unknown exception caught";
  }

  return EXIT_FAILURE;
}
