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

#include "Edge/edgecontrollerclient.h"
#include "Edge/edgedispatcher.h"
#include "Edge/edgelambdaprocessoroptions.h"
#include "Edge/edgeserverimpl.h"
#include "Edge/edgeserverimplfactory.h"
#include "Edge/forwardingtableserver.h"
#include "Edge/ptimeestimator.h"
#include "Support/conf.h"
#include "Support/glograii.h"

#include <glog/logging.h>

#include <boost/program_options.hpp>

#include <cstdlib>

namespace po = boost::program_options;
namespace ec = uiiit::edge;

int main(int argc, char* argv[]) {
  uiiit::support::GlogRaii myGlogRaii(argv[0]);

  std::string myPtimeEstimatorConf;
  std::string myServerConf;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
  ("server-conf",
   po::value<std::string>(&myServerConf)->default_value("type=grpc"),
   "Comma-separated configuration of the server.")
  ("ptimeest-conf",
   po::value<std::string>(&myPtimeEstimatorConf)->default_value("type=rtt,window-size=50,stale-period=10"),
   "Comma-separated configuration of the processing time estimator.")
  ;
  // clang-format on

  try {
    uiiit::edge::EdgeLambdaProcessorOptions myCli(argc, argv, myDesc);

    if (myCli.serverEndpoint().empty()) {
      throw std::runtime_error("Empty edge router end-point: " +
                               myCli.serverEndpoint());
    }
    if (myCli.forwardingEndpoint().empty()) {
      throw std::runtime_error("Empty forwarding table end-point: " +
                               myCli.forwardingEndpoint());
    }

    const auto myServerImplConf = uiiit::support::Conf(myServerConf);

    ec::EdgeDispatcher myEdgeDispatcher(
        myCli.serverEndpoint(),
        myCli.forwardingEndpoint(),
        myCli.controllerEndpoint(),
        myCli.secure(),
        uiiit::support::Conf(myCli.routerConf()),
        uiiit::support::Conf(myPtimeEstimatorConf),
        myServerImplConf);

    const auto myServerImpl =
        ec::EdgeServerImplFactory::make(myEdgeDispatcher,
                                        myCli.numThreads(),
                                        myCli.secure(),
                                        uiiit::support::Conf(myServerConf));

    auto myTables = myEdgeDispatcher.tables();
    assert(myTables.size() == 1);
    assert(myTables[0] != nullptr);

    fakeFill(*myTables[0], myCli.fakeNumLambdas(), myCli.fakeNumDestinations());

    ec::ForwardingTableServer myForwardingTableServer(
        myCli.forwardingEndpoint(), *myTables[0]);

    myForwardingTableServer.run(false); // non-blocking
    myServerImpl->run();
    myServerImpl->wait();

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
