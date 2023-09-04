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

#include "Edge/composer.h"
#include "Edge/computer.h"
#include "Edge/edgecomputerhttp.h"
#include "Edge/edgecomputerserver.h"
#include "Edge/edgecomputersim.h"
#include "Edge/edgecontrollerclient.h"
#include "Edge/edgeserverimpl.h"
#include "Edge/edgeserverimplfactory.h"
#include "Edge/edgeserveroptions.h"
#include "Edge/stateserver.h"
#include "Support/conf.h"
#include "Support/glograii.h"
#include "Support/signalhandlerwait.h"

#include <glog/logging.h>

#include <boost/program_options.hpp>

#include <cstdlib>
#include <stdexcept>

namespace po = boost::program_options;
namespace ec = uiiit::edge;
namespace us = uiiit::support;

int main(int argc, char* argv[]) {
  us::GlogRaii          myGlogRaii(argv[0]);
  us::SignalHandlerWait mySignalHandler;

  std::string myUtilServerEndpoint;
  std::string myConf;
  std::string myServerConf;
  std::string myComputerType;
  std::string myCompanionEndpoint;
  std::string myStateEndpoint;
  std::string myHttpConfStr;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
  ("server-conf",
   po::value<std::string>(&myServerConf)->default_value("type=grpc"),
   "Comma-separated configuration of the server.")
  ("computer-type",
   po::value<std::string>(&myComputerType)->default_value("sim"),
   "Edge computer type (one of: sim, http).")
  ("utilization-endpoint",
   po::value<std::string>(&myUtilServerEndpoint)->default_value("0.0.0.0:6476"),
   "Utilization server end-point. If empty utilization is not computed.")
  ("asynchronous",
   "If used makes the edge computer asynchronous.")
  ("companion-endpoint",
   po::value<std::string>(&myCompanionEndpoint)->default_value(""),
   "Companion server end-point, required to serve function chains. Makes the computer asynchronous.")
  ("state-endpoint",
   po::value<std::string>(&myStateEndpoint)->default_value(""),
   "Use the given end-point to get/set the states. If the --no-state-server option is not specified, then a state server is also created listening at this end-point.")
  ("no-state-server", "Do not create a state server. Only makes sense if --state-endpoint is not empty.")
  ("conf",
   po::value<std::string>(&myConf)->default_value(
     "type=raspberry,"
     "num-cpu-containers=1,"
     "num-cpu-workers=4,"
     "num-gpu-containers=1,"
     "num-gpu-workers=2"),
   "Computer configuration. Use type=file,path=<myfile.json> to read configuration from file. Used only with --computer-type sim")
  ("http-conf",
   po::value<std::string>(&myHttpConfStr)->default_value("gateway-url=http://localhost:8080/,num-clients=5,type=OpenFaaS(0.8)"),
   "Edge computer type (one of: sim, http). Used only with --computer-type http")
  ("json-example", "Dump an JSON configuration file and exit.")
  ;
  // clang-format on

  try {
    uiiit::edge::EdgeServerOptions myCli(argc, argv, myDesc);

    if (myCli.varMap().count("json-example") > 0) {
      std::cout << ec::Composer::jsonExample() << std::endl;
      return EXIT_SUCCESS;
    }

    if (myCli.serverEndpoint().empty()) {
      throw std::runtime_error("Empty end-point: " + myCli.serverEndpoint());
    }

    if (myStateEndpoint.empty() and myCli.varMap().count("no-state-server")) {
      throw std::runtime_error(
          "Cannot specify --no-state-server without --state-endpoint");
    }

    const auto myAsynchronous = myCli.varMap().count("asynchronous") > 0 ||
                                not myCompanionEndpoint.empty();

    std::unique_ptr<ec::EdgeComputer>       myEdgeComputer;
    std::unique_ptr<ec::EdgeComputerServer> myUtilServer;
    if (myComputerType == "sim") {

      ec::Computer::UtilCallback myUtilCallback;
      if (not myUtilServerEndpoint.empty()) {
        myUtilServer.reset(new ec::EdgeComputerServer(myUtilServerEndpoint));
        myUtilCallback =
            [&myUtilServer](const std::map<std::string, double>& aUtil) {
              myUtilServer->add(aUtil);
            };
        myUtilServer->run(false); // non-blocking
      }

      auto myEdgeComputerSim = std::make_unique<ec::EdgeComputerSim>(
          myAsynchronous ? myCli.numThreads() : 0,
          myCli.serverEndpoint(),
          myCli.secure(),
          myUtilCallback);
      ec::Composer()(us::Conf(myConf), myEdgeComputerSim->computer());

      if (myCli.controllerEndpoint().empty()) {
        VLOG(1) << "No controller specified: announce disabled";
      } else {
        ec::EdgeControllerClient myControllerClient(myCli.controllerEndpoint());
        myControllerClient.announceComputer(
            myCli.serverEndpoint(),
            *myEdgeComputerSim->computer().containerList());
        LOG(INFO) << "Announced to " << myCli.controllerEndpoint();
      }

      myEdgeComputer.reset(myEdgeComputerSim.release());

    } else if (myComputerType == "http") {
      const us::Conf myHttpConf(myHttpConfStr);
      myEdgeComputer = std::make_unique<ec::EdgeComputerHttp>(
          myAsynchronous ? myCli.numThreads() : 0,
          myHttpConf.getUint("num-clients"),
          myCli.serverEndpoint(),
          myCli.secure(),
          ec::edgeComputerHttpTypeFromString(myHttpConf("type")),
          myHttpConf("gateway-url"));
    }

    if (not myEdgeComputer) {
      throw std::runtime_error("unknown edge computer type: " + myComputerType);
    }

    if (not myCompanionEndpoint.empty()) {
      myEdgeComputer->companion(myCompanionEndpoint);
    }

    std::unique_ptr<ec::StateServer> myStateServer;
    if (not myStateEndpoint.empty() and
        myCli.varMap().count("no-state-server") == 0) {
      myStateServer = std::make_unique<ec::StateServer>(myStateEndpoint);
      myStateServer->run(false);
    }
    myEdgeComputer->state(myStateEndpoint); // end-point can be empty

    const auto myServerImpl =
        ec::EdgeServerImplFactory::make(*myEdgeComputer,
                                        myCli.numThreads(),
                                        myCli.secure(),
                                        us::Conf(myServerConf));

    myServerImpl->run();    // non-blocking
    mySignalHandler.wait(); // blocking

    // perform clean exit by removing this computer from the controller
    if (not myCli.controllerEndpoint().empty()) {
      ec::EdgeControllerClient myControllerClient(myCli.controllerEndpoint());
      myControllerClient.removeComputer(myCli.serverEndpoint());
    }

    return EXIT_SUCCESS;

  } catch (const us::CliExit&) {
    return EXIT_SUCCESS; // clean exit

  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Exception caught: " << aErr.what();

  } catch (...) {
    LOG(ERROR) << "Unknown exception caught";
  }

  return EXIT_FAILURE;
}
