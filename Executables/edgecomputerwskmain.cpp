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

#include "Edge/edgecomputerwsk.h"
#include "Edge/edgecontrollermessages.h"  // ContainerList
#include "Edge/edgecontrollerclient.h"
#include "Edge/edgeserveroptions.h"
#include "Edge/edgeservergrpc.h"
#include "OpenWhisk/lister.h"
#include "Support/conf.h"
#include "Support/glograii.h"
#include "Support/periodictask.h"
#include "Support/signalhandlerwait.h"

#include <glog/logging.h>

#include <boost/program_options.hpp>

#include <cstdlib>

namespace po = boost::program_options;
namespace ec = uiiit::edge;

int main(int argc, char* argv[]) {
  uiiit::support::GlogRaii          myGlogRaii(argv[0]);
  uiiit::support::SignalHandlerWait mySignalHandler;

  std::string myWskApiRoot;
  std::string myWskAuth;
  std::string myServerConf;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
  ("server-conf",
   po::value<std::string>(&myServerConf)->default_value("type=grpc"),
   "Comma-separated configuration of the server.")
  ("wsk-api-root",
   po::value<std::string>(&myWskApiRoot)->default_value("https://127.0.0.1:80"),
   "OpenWhisk API root")
  ("wsk-auth",
   po::value<std::string>(&myWskAuth)->default_value(""),
   "OpenWhisk basic authentication token")
  ;
  // clang-format on

  try {
    uiiit::edge::EdgeServerOptions myCli(argc, argv, myDesc);

    if (myCli.serverEndpoint().empty()) {
      throw std::runtime_error("Empty end-point: " + myCli.serverEndpoint());
    }

    std::unique_ptr<uiiit::support::PeriodicTask>       myListerTask;
    std::unique_ptr<ec::EdgeControllerClient>           myControllerClient;
    std::unique_ptr<uiiit::wsk::Lister>                 myLister;
    std::map<uiiit::wsk::ActionKey, uiiit::wsk::Action> myActions;

    if (myCli.controllerEndpoint().empty()) {
      VLOG(1) << "No controller specified: announce disabled";

    } else {
      VLOG(2) << "Creating the edge controller client towards "
              << myCli.controllerEndpoint();
      myControllerClient = std::make_unique<ec::EdgeControllerClient>(
          myCli.controllerEndpoint());

      VLOG(2) << "Creating the OpenWhisk action lister from " << myWskApiRoot
              << " with basic auth token " << myWskAuth;
      myLister = std::make_unique<uiiit::wsk::Lister>(myWskApiRoot, myWskAuth);

      VLOG(2) << "Creating the OpenWhisk lister task";
      myListerTask = std::make_unique<uiiit::support::PeriodicTask>(
          [&myCli,
           &myControllerClient,
           &myWskApiRoot,
           &myLister,
           &myActions]() {
            assert(myLister.get() != nullptr);

            // retrieve the actions
            auto myNewActions = (*myLister)(0, 0); // no limit, no skip

            // if there are changes, then re-announce the computer
            if (uiiit::wsk::sameKeys(myActions, myNewActions)) {
              return;
            }
            myActions.swap(myNewActions);
            LOG(INFO) << "Set of actions at " << myWskApiRoot
                      << " changed, announcing to "
                      << myCli.controllerEndpoint();

            ec::ContainerList myContainerList;
            for (const auto& elem : myActions) {
              myContainerList.theContainers.emplace_back(
                  ec::ContainerList::Container{
                      "wsk", "wsk", elem.first.toString(), 1});
            }

            assert(myControllerClient.get() != nullptr);
            myControllerClient->removeComputer(myCli.serverEndpoint());
            myControllerClient->announceComputer(myCli.serverEndpoint(),
                                                 myContainerList);
          },
          1.0);
    }

    ec::EdgeComputerWsk myServer(
        myCli.serverEndpoint(), myWskApiRoot, myWskAuth);

    std::unique_ptr<ec::EdgeServerImpl> myServerImpl;
    const auto myServerImplConf = uiiit::support::Conf(myServerConf);

    if (myServerImplConf("type") == "grpc") {
      myServerImpl.reset(new ec::EdgeServerGrpc(myServer, myServerImplConf("server-endpoint"), myServerImplConf.getInt("num-threads")));
    } else if (myServerImplConf("type") == "quic") {
      // myServerImpl.reset(new ec::EdgeServerQuic()); // Costruttore da mettere parametri makeHqParams(myImplConf)
      LOG(INFO) << "COSTRUTTORE EDGE SERVER QUIC" << '\n';
    } else {
      throw std::runtime_error("EdgeServer type not allowed: " +
                               myServerImplConf("type"));
    }
    assert(myServerImpl != nullptr);

    myServerImpl -> run();
    mySignalHandler.wait(); // blocking

    // perform clean exit by removing this computer from the controller
    if (myControllerClient) {
      myListerTask.reset();
      myControllerClient->removeComputer(myCli.serverEndpoint());
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
