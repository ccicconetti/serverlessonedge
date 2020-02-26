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

#include "trivialedgecontrollerinstaller.h"

#include <sstream>

namespace uiiit {
namespace edge {

TrivialEdgeControllerInstaller::TrivialEdgeControllerInstaller()
    : EdgeControllerInstaller()
    , theLog()
    , theDisconnected() {
}

bool TrivialEdgeControllerInstaller::changeRoutes(
    const std::string& aEndpoint,
    const std::list<std::tuple<std::string, std::string, float, bool>>&
        aLambdas) {
  if (theDisconnected == "*" or aEndpoint == theDisconnected) {
    return false;
  }

  std::stringstream myStream;

  myStream << aEndpoint << ":";
  for (const auto& myLambda : aLambdas) {
    myStream << ' ' << std::get<0>(myLambda) << ',' << std::get<1>(myLambda)
             << ',' << std::get<2>(myLambda) << ','
             << (std::get<3>(myLambda) ? "F" : "");
  }
  myStream << '\n';

  theLog += myStream.str();

  return true;
}

bool TrivialEdgeControllerInstaller::removeRoutes(
    const std::string&            aEdgeRouterEndpoint,
    const std::string&            aEdgeComputerEndpoint,
    const std::list<std::string>& aLambdas) {
  if (theDisconnected == "*" or aEdgeRouterEndpoint == theDisconnected) {
    return false;
  }

  std::stringstream myStream;

  myStream << "del " << aEdgeRouterEndpoint << ' ' << aEdgeComputerEndpoint;
  for (const auto& myLambda : aLambdas) {
    myStream << ' ' << myLambda;
  }
  myStream << '\n';

  theLog += myStream.str();

  return true;
}

bool TrivialEdgeControllerInstaller::flushRoutes(
    const std::string& aEdgeRouterEndpoint) {
  if (theDisconnected == "*" or aEdgeRouterEndpoint == theDisconnected) {
    return false;
  }

  std::stringstream myStream;
  myStream << "flush " << aEdgeRouterEndpoint << '\n';

  theLog += myStream.str();

  return true;
}

  std::string TrivialEdgeControllerInstaller::logGetAndClear() {
    std::string myRet;
    myRet.swap(theLog);
    return myRet;
  }

////////////////////////////////////////////////////////////////////////////////
// free functions

ContainerList makeContainers(size_t aNumLambdas, const size_t aStartingFrom) {
  ContainerList myRet;
  for (size_t i = 0; i < aNumLambdas; i++) {
    myRet.theContainers.push_back(ContainerList::Container{
        "cont0",
        "cpu0",
        std::string("lambda") + static_cast<char>('0' + aStartingFrom + i),
        2});
  }
  return myRet;
}

} // namespace edge
} // namespace uiiit
