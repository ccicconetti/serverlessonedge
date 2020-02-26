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

#include "Edge/edgecontrolleretsi.h"
#include "Edge/edgecontrolleretsibalanced.h"
#include "Edge/edgecontrollerflat.h"
#include "Edge/edgecontrollerhier.h"
#include "Edge/edgecontrollerrpc.h"
#include "Edge/edgecontrollerserver.h"
#include "Edge/topology.h"
#include "EtsiMec/etsimecoptions.h"
#include "EtsiMec/staticfileueapplcmproxy.h"
#include "Support/glograii.h"
#include "Support/tostring.h"

#include <glog/logging.h>

#include <boost/program_options.hpp>

#include <cstdlib>
#include <functional>
#include <set>
#include <string>

namespace po = boost::program_options;
namespace ec = uiiit::edge;

int main(int argc, char* argv[]) {
  uiiit::support::GlogRaii myGlogRaii(argv[0]);

  std::string             myServerEndpoint;
  std::string             myType;
  std::string             myInstallerType;
  std::string             myUeAppLcmProxyFile;
  std::string             myBalancedEndpoint;
  double                  myBalancedPeriod;
  std::string             myTopologyFile;
  std::string             myHierObjective;
  po::options_description myDesc("Allowed options");

  // clang-format off
  myDesc.add_options()
  ("server-endpoint",
   po::value<std::string>(&myServerEndpoint)->default_value("0.0.0.0:6475"),
   "Edge controller server end-point.")
  ("type",
   po::value<std::string>(&myType)->default_value("plain"),
   "Controller type, one of: plain, etsi-file, etsi-balanced.")
  ("installer-type",
   po::value<std::string>(&myInstallerType)->default_value("flat"),
   "Table installer type, one of: flat, hier.")
  ("file",
   po::value<std::string>(&myUeAppLcmProxyFile)->default_value(""),
   "Configuration file of the ETSI UE application LCM proxy, required with type etsi-file.")
  ("balanced-endpoint",
   po::value<std::string>(&myBalancedEndpoint)->default_value(""),
   "End-point of the remote ETSI UE application LCM proxy, required with type etsi-balanced.")
  ("balanced-period",
   po::value<double>(&myBalancedPeriod)->default_value(5.0),
   "Optimization period, used with type etsi-balanced.")
  ("topology-file",
   po::value<std::string>(&myTopologyFile)->default_value("dist.txt"),
   "Topology file, only meaningful if installer-type is hier.")
  ("topology-randomized",
   "If specified the distances in the topology file are randomized.")
  ("hier-objective",
   po::value<std::string>(&myHierObjective)->default_value("min-max"),
   "Objective function to use with a hierarchical controller.")
  ;
  // clang-format on

  try {
    ::uiiit::etsimec::EtsiMecOptions myCli(
        argc, argv, "http://127.0.0.1:6500", false, myDesc);

    if (myServerEndpoint.empty()) {
      throw std::runtime_error("Empty edge controller end-point: " +
                               myServerEndpoint);
    }

    const static std::set<std::string> myAllowedTypes(
        {"plain", "etsi-file", "etsi-balanced"});
    if (myAllowedTypes.count(myType) == 0) {
      throw std::runtime_error(
          "Invalid type '" + myType +
          "', choose one in: " + toString(myAllowedTypes, ","));
    }

    const static std::set<std::string> myAllowedInstallers({"flat", "hier"});
    if (myAllowedInstallers.count(myInstallerType) == 0) {
      throw std::runtime_error(
          "Invalid installer type '" + myInstallerType +
          "', choose one in: " + toString(myAllowedInstallers, ","));
    }

    ec::EdgeControllerServer myServer(myServerEndpoint);

    std::unique_ptr<uiiit::etsimec::StaticUeAppLcmProxy> myEtsiProxy;
    std::unique_ptr<ec::EdgeController>                  myEdgeControllerEtsi;
    if (myType == "etsi-file") {
      if (myCli.apiRoot().empty()) {
        throw std::runtime_error("The ETSI MEC API root cannot be empty");
      }
      if (myUeAppLcmProxyFile.empty()) {
        throw std::runtime_error("ETSI UE app LCM proxy file not specified");
      }
      myEtsiProxy.reset(new uiiit::etsimec::StaticFileUeAppLcmProxy(
          myCli.apiRoot(), myUeAppLcmProxyFile));
      LOG(INFO)
          << "Loaded an ETSI UE application LCM proxy configuration file with "
          << myEtsiProxy->numAddresses() << " entries";
      myEdgeControllerEtsi.reset(new ec::EdgeControllerEtsi(*myEtsiProxy));
      myEtsiProxy->start();

    } else if (myType == "etsi-balanced") {
      if (myCli.apiRoot().empty()) {
        throw std::runtime_error("The ETSI MEC API root cannot be empty");
      }
      if (myBalancedEndpoint.empty()) {
        throw std::runtime_error(
            "ETSI MEC UE app LCM proxy end-point not specified");
      }
      myEdgeControllerEtsi.reset(new ec::EdgeControllerEtsiBalanced(
          myBalancedEndpoint, myBalancedPeriod));
    } else {
      assert(myType == "plain");
    }

    if (myEdgeControllerEtsi) {
      myServer.subscribe(std::move(myEdgeControllerEtsi));
    }

    std::unique_ptr<ec::EdgeController> myEdgeControllerRpc;
    if (myInstallerType == "flat") {
      myEdgeControllerRpc.reset(
          new ec::EdgeControllerRpc<ec::EdgeControllerFlat>());
    } else {
      assert(myInstallerType == "hier");
      auto myTopology = std::make_unique<ec::Topology>(myTopologyFile);
      if (myCli.varMap().count("topology-randomized") > 0) {
        myTopology->randomize();
      }
      auto myController =
          std::make_unique<ec::EdgeControllerRpc<ec::EdgeControllerHier>>();
      myController->objective(ec::objectiveFromString(myHierObjective));
      myController->loadTopology(std::move(myTopology));
      myEdgeControllerRpc = std::move(myController);
    }
    myServer.subscribe(std::move(myEdgeControllerRpc));

    myServer.run(true); // blocking

    return EXIT_SUCCESS;

  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Exception caught: " << aErr.what();

  } catch (...) {
    LOG(ERROR) << "Unknown exception caught";
  }

  return EXIT_FAILURE;
}
