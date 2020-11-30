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

#include "Edge/composer.h"
#include "Edge/computer.h"
#include "Edge/edgecomputer.h"
#include "Edge/edgecomputerserver.h"
#include "Edge/edgecontrollerclient.h"
#include "Edge/edgeservergrpc.h"
//#include "Edge/edgeserverquic"
#include "Edge/edgeserveroptions.h"
#include "Support/conf.h"
#include "Support/glograii.h"
#include "Support/signalhandlerwait.h"

#include <glog/logging.h>

#include <boost/program_options.hpp>

#include <cstdlib>

namespace po = boost::program_options;
namespace ec = uiiit::edge;

int main(int argc, char* argv[]) {
  uiiit::support::GlogRaii          myGlogRaii(argv[0]);
  uiiit::support::SignalHandlerWait mySignalHandler;

  std::string myUtilServerEndpoint;
  std::string myConf;
  std::string myServerConf;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
  ("server-conf",
   po::value<std::string>(&myServerConf)->default_value("type=grpc"),
   "Comma-separated configuration of the server.") // endpoint, numthreads OR string of conf parameters to build HQParams
  ("utilization-endpoint",
   po::value<std::string>(&myUtilServerEndpoint)->default_value("0.0.0.0:6476"),
   "Utilization server end-point. If empty utilization is not computed.")
  ("conf",
   po::value<std::string>(&myConf)->default_value(
     "type=raspberry,"
     "num-cpu-containers=1,"
     "num-cpu-workers=4,"
     "num-gpu-containers=1,"
     "num-gpu-workers=2"),
   "Computer configuration. Use type=file,path=<myfile.json> to read configuration from file")
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

    ec::Computer::UtilCallback              myUtilCallback;
    std::unique_ptr<ec::EdgeComputerServer> myUtilServer;
    if (not myUtilServerEndpoint.empty()) {
      myUtilServer.reset(new ec::EdgeComputerServer(myUtilServerEndpoint));
      myUtilCallback =
          [&myUtilServer](const std::map<std::string, double>& aUtil) {
            myUtilServer->add(aUtil);
          };
      myUtilServer->run(false); // non-blocking
    }

    ec::EdgeComputer myServer(myCli.serverEndpoint(), myUtilCallback);

    std::unique_ptr<ec::EdgeServerImpl> myServerImpl;
    const auto myServerImplConf = uiiit::support::Conf(myServerConf);

    if (myServerImplConf("type") == "grpc") {
      myServerImpl.reset(new ec::EdgeServerGrpc(
          myServer, myCli.serverEndpoint(), myCli.numThreads()));
    } else if (myServerImplConf("type") == "quic") {
      // myServerImpl.reset(new ec::EdgeServerQuic()); // Costruttore da mettere
      // parametri makeHqParams(myImplConf)
      LOG(INFO) << "COSTRUTTORE EDGE SERVER QUIC" << '\n';
    } else {
      throw std::runtime_error("EdgeServer type not allowed: " +
                               myServerImplConf("type"));
    }
    assert(myServerImpl != nullptr);

    ec::Composer()(uiiit::support::Conf(myConf), myServer.computer());

    if (myCli.controllerEndpoint().empty()) {
      VLOG(1) << "No controller specified: announce disabled";
    } else {
      ec::EdgeControllerClient myControllerClient(myCli.controllerEndpoint());
      myControllerClient.announceComputer(myCli.serverEndpoint(),
                                          *myServer.computer().containerList());
      LOG(INFO) << "Announced to " << myCli.controllerEndpoint();
    }

    myServerImpl->run();
    // myServer.run(); // non-blocking
    // myServer.wait(); // blocking
    mySignalHandler.wait(); // blocking

    // perform clean exit by removing this computer from the controller
    if (not myCli.controllerEndpoint().empty()) {
      ec::EdgeControllerClient myControllerClient(myCli.controllerEndpoint());
      myControllerClient.removeComputer(myCli.serverEndpoint());
    }

    return EXIT_SUCCESS;

  } catch (const uiiit::support::CliExit&) {
    return EXIT_SUCCESS; // clean exit

  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Exception caught: " << aErr.what();

  } catch (...) {
    LOG(ERROR) << "Unknown exception caught";
  }

  return EXIT_FAILURE;
}
