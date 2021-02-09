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

#include "Edge/forwardingtableclient.h"
#include "Support/glograii.h"

#include <boost/program_options.hpp>

#include <glog/logging.h>

#include <cstdlib>

namespace po = boost::program_options;
namespace ec = uiiit::edge;

int main(int argc, char* argv[]) {
  uiiit::support::GlogRaii myGlogRaii(argv[0]);

  std::string myServerEndpoint;
  std::string myAction;
  std::string myLambda;
  std::string myDestination;
  float       myWeight;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
    ("help,h", "produce help message")
    ("server-endpoint",
     po::value<std::string>(&myServerEndpoint)->default_value("localhost:6474"),
     "Forwarding table server end-point.")
    ("action",
     po::value<std::string>(&myAction)->default_value("dump"),
     "Action. One of: dump, reset, flush, change, remove.")
    ("lambda",
     po::value<std::string>(&myLambda)->default_value("clambda0"),
     "Lambda (only meaningful with change and remove actions).")
    ("destination",
     po::value<std::string>(&myDestination)->default_value("localhost:6473"),
     "Destination end-point (only meaningful with change and remove actions).")
    ("weight",
     po::value<float>(&myWeight)->default_value(1.0f),
     "Weight (only meaningful with change actions).")
    ("final","Set to make this route final (only valid with change action).")
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

    ec::ForwardingTableClient myClient(myServerEndpoint);

    const auto myFinal = myVarMap.count("final") > 0;

    auto myPrintOk = true;
    if (myAction == "dump") {
      myPrintOk = false;
      std::cout << myClient.dump();
    } else if (myAction == "reset") {
      //
      // for all tables reset the weight to the initial value of 1
      // since multiple tables may contain the same <lambda,destination>
      // we keep track of those already updated to avoid doing it
      // multiple times for the same pair
      //
      // note that the forward flag is kept the same: this mechanism
      // does _not_ support having multiple <lambda,destination> pairs
      // with DIFFERENT forward flags in different tables, for which
      // we do not foresee a use case at the moment
      //
      std::map<std::string, std::set<std::string>> myUpdated;
      for (auto i = 0u; i < myClient.numTables(); i++) {
        for (const auto& myEntry : myClient.table(i)) {
          auto& myDestinations = myUpdated[myEntry.first];
          for (const auto& myDestination : myEntry.second) {
            if (myDestinations.insert(myDestination.first).second) {
              VLOG(1) << "Updating weight of lambda " << myEntry.first
                      << " with "
                      << (myDestination.second.second ? "final " : "")
                      << "destination " << myDestination.first << " from "
                      << myDestination.second.first << " to 1.0";
              myClient.change(myEntry.first,
                              myDestination.first,
                              1.0,
                              myDestination.second.second);
            }
          }
        }
      }
    } else if (myAction == "flush") {
      myClient.flush();
    } else if (myAction == "change") {
      myClient.change(myLambda, myDestination, myWeight, myFinal);
    } else if (myAction == "remove") {
      myClient.remove(myLambda, myDestination);
    } else {
      throw std::runtime_error("Invalid action " + myAction +
                               ", choose one of: dump, flush, change, remove");
    }

    if (myPrintOk) {
      std::cout << "OK" << std::endl;
    }

    return EXIT_SUCCESS;

  } catch (const std::exception& aErr) {
    std::cerr << "Exception caught: " << aErr.what() << std::endl;

  } catch (...) {
    std::cerr << "Unknown exception caught" << std::endl;
  }

  return EXIT_FAILURE;
}
