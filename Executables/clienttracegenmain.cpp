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

#include "Support/glograii.h"

#include <boost/program_options.hpp>

#include <glog/logging.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <random>

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  uiiit::support::GlogRaii myGlogRaii(argv[0]);

  std::string myServerEndpoint;
  std::string myLambda;
  size_t      myMinSize;
  size_t      myMaxSize;
  double      myRate;
  size_t      myNumClients;
  double      myStartingTime;
  double      myDuration;
  std::string myOutfile;
  size_t      mySeedUser;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
    ("help,h", "produce help message")
    ("server-endpoint",
     po::value<std::string>(&myServerEndpoint)->default_value("localhost:6473"),
     "Server end-point.")
    ("lambda",
     po::value<std::string>(&myLambda)->default_value("clambda0"),
     "Lambda function name.")
    ("min-size",
     po::value<size_t>(&myMinSize)->default_value(100),
     "Minimum request size.")
    ("max-size",
     po::value<size_t>(&myMaxSize)->default_value(500),
     "Maximum request size.")
    ("rate",
     po::value<double>(&myRate)->default_value(1.0),
     "Request rate per client.")
    ("num-clients",
     po::value<size_t>(&myNumClients)->default_value(5),
     "Number of clients generating requests.")
    ("starting-time",
     po::value<double>(&myStartingTime)->default_value(0.0),
     "Trace starting time, in s.")
    ("duration",
     po::value<double>(&myDuration)->default_value(10.0),
     "Trace duration, in s.")
    ("output-file",
     po::value<std::string>(&myOutfile)->default_value("/dev/stdout"),
     "Output filename.")
    ("append", "Append to output, do not overwrite.")
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

    if (myServerEndpoint.empty()) {
      throw std::runtime_error("Empty end-point");
    }
    if (myLambda.empty()) {
      throw std::runtime_error("Empty lambda");
    }
    if (myNumClients == 0) {
      throw std::runtime_error("Zero clients");
    }
    if (myRate <= 0) {
      throw std::runtime_error("Invalid request rate: " +
                               std::to_string(myRate));
    }
    if (myMaxSize < myMinSize) {
      throw std::runtime_error("Invalid max size " + std::to_string(myMaxSize) +
                               " < min size " + std::to_string(myMinSize));
    }

    auto myOutMode = std::ofstream::out;
    if (myVarMap.count("append")) {
      myOutMode |= std::ofstream::app;
    }
    std::ofstream myOut(myOutfile, myOutMode);
    if (not myOut.is_open()) {
      throw std::runtime_error("Could not open '" + myOutfile + "'");
    }

    struct Event {
      double theTime;
      size_t theSize;
      bool   operator<(const Event& aOther) const noexcept {
        return theTime < aOther.theTime;
      }
    };

    std::list<Event> myEvents;

    std::seed_seq                         mySeed{{mySeedUser}};
    std::mt19937                          myGenerator{mySeed};
    std::uniform_int_distribution<size_t> myUniRv(myMinSize, myMaxSize);
    std::exponential_distribution<double> myExpRv(myRate);

    for (size_t i = 0; i < myNumClients; i++) {
      double myNextTime = myExpRv(myGenerator);
      while (myNextTime < myDuration) {
        myEvents.emplace_back(
            Event{myStartingTime + myNextTime, myUniRv(myGenerator)});
        myNextTime += myExpRv(myGenerator);
      }
    }

    myEvents.sort();

    for (const auto& myEvent : myEvents) {
      myOut << myEvent.theTime << ' ' << myLambda << ' ' << myServerEndpoint
            << ' ' << myEvent.theSize << '\n';
    }

    return EXIT_SUCCESS;
  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Exception caught: " << aErr.what();
  } catch (...) {
    LOG(ERROR) << "Unknown exception caught";
  }

  return EXIT_FAILURE;
}
