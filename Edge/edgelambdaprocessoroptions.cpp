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

#include "edgelambdaprocessoroptions.h"

#include "Edge/edgelambdaprocessor.h"

namespace uiiit {
namespace edge {

EdgeLambdaProcessorOptions::EdgeLambdaProcessorOptions(
    int argc, char** argv, boost::program_options::options_description& aDesc)
    : support::CliOptions(argc, argv, aDesc)
    , theServerEndpoint()
    , theControllerEndpoint()
    , theNumThreads() {
  // clang-format off
  theDesc.add_options()
  ("server-endpoint",
   boost::program_options::value<std::string>(&theServerEndpoint)
     ->default_value("0.0.0.0:6473"),
   "Edge router server end-point.")
  ("controller-endpoint",
   boost::program_options::value<std::string>(&theControllerEndpoint)
     ->default_value(""),
   "If specified announce to this controller.")
  ("num-threads",
   boost::program_options::value<size_t>(&theNumThreads)
     ->default_value(5),
   "Number of threads spawned in the edge router.")

  ("configuration-endpoint",
   boost::program_options::value<std::string>(&theForwardingEndpoint)->default_value("0.0.0.0:6474"),
   "Forwarding table configuration end-point.")
  ("router-conf",
   boost::program_options::value<std::string>(&theRouterConf)->default_value(EdgeLambdaProcessor::defaultConf()),
   "Comma-separated configuration of the edge router.")
  ("fake-num-lambdas",
   boost::program_options::value<size_t>(&theFakeNumLambdas)
     ->default_value(0),
   "Number of fake lambdas to pre-load.")
  ("fake-num-destinations",
   boost::program_options::value<size_t>(&theFakeNumDestinations)
     ->default_value(0),
   "Number of fake destinations per lambda to pre-load.")
  ;
  // clang-format on
  parse();
}

} // namespace edge
} // namespace uiiit
