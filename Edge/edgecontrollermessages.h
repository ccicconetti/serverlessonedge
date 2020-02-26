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

#pragma once

#include <iostream>
#include <list>
#include <string>

#include "edgecontroller.grpc.pb.h"

namespace uiiit {
namespace edge {

struct ContainerList {
  struct Container {
    std::string theName;
    std::string theProcessor;
    std::string theLambda;
    uint32_t    theNumWorkers;

    bool operator==(const Container& aOther) const {
      return theName == aOther.theName and
             theProcessor == aOther.theProcessor and
             theLambda == aOther.theLambda and
             theNumWorkers == aOther.theNumWorkers;
    }
  };
  std::list<Container> theContainers;

  bool operator==(const ContainerList& aOther) const {
    return theContainers == aOther.theContainers;
  }
};

rpc::ComputerInfo toProtobuf(const std::string&   aEdgeServerEndpoint,
                             const ContainerList& aContainers);

ContainerList containerListFromProtobuf(const rpc::ComputerInfo& aMessage);

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                     aStream,
                         const uiiit::edge::ContainerList& aContainerList);
