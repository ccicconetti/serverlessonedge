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

#include "Edge/wskproxy.h"
#include "Support/clioptions.h"
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

  std::string myWskApiRoot;
  std::string myServerEndpoint;
  size_t      myNumThreads;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
  ("wsk-api-root",
   po::value<std::string>(&myWskApiRoot)->default_value("http://127.0.0.1:20000"),
   "OpenWhisk API root")
  ("server-endpoint",
   boost::program_options::value<std::string>(&myServerEndpoint)
     ->default_value("127.0.0.1:6473"),
   "Edge server end-point.")
  ("num-threads",
   boost::program_options::value<size_t>(&myNumThreads)
     ->default_value(5),
   "Maximum Number of threads spawned.")
   ("secure", "If specified use SSL/TLS authentication.")
  ;
  // clang-format on

  try {
    uiiit::support::TrivialOptions myCli(argc, argv, myDesc);

    ec::WskProxy myProxy(myWskApiRoot,
                         myServerEndpoint,
                         myCli.varMap().count("secure") == 1,
                         myNumThreads);

    myProxy.start();
    mySignalHandler.wait(); // blocking

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
