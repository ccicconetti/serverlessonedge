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

#include "edgecontrollermessages.h"

namespace uiiit {
namespace edge {

rpc::ComputerInfo toProtobuf(const std::string&   aEdgeServerEndpoint,
                             const ContainerList& aContainers) {
  rpc::ComputerInfo myRet;
  myRet.set_edgeserverendpoint(aEdgeServerEndpoint);
  auto myNdx = 0;
  for (const auto& myContainer : aContainers.theContainers) {
    myRet.add_containers();
    auto& myNewContainer = *myRet.mutable_containers(myNdx);
    myNewContainer.set_name(myContainer.theName);
    myNewContainer.set_processor(myContainer.theProcessor);
    myNewContainer.set_lambda(myContainer.theLambda);
    myNewContainer.set_numworkers(myContainer.theNumWorkers);
    ++myNdx;
  }
  return myRet;
}

ContainerList containerListFromProtobuf(const rpc::ComputerInfo& aMessage) {
  ContainerList myRet;
  for (auto i = 0; i < aMessage.containers_size(); i++) {
    const auto& myContainer = aMessage.containers(i);
    myRet.theContainers.emplace_back(
        ContainerList::Container{myContainer.name(),
                                 myContainer.processor(),
                                 myContainer.lambda(),
                                 myContainer.numworkers()});
  }
  return myRet;
}

} // namespace edge
} // namespace uiiit

std::ostream& operator<<(std::ostream&                     aStream,
                         const uiiit::edge::ContainerList& aContainerList) {
  for (const auto& myContainer : aContainerList.theContainers) {
    aStream << myContainer.theName << ": processor " << myContainer.theProcessor
            << ", lambda " << myContainer.theLambda << ", #workers "
            << myContainer.theNumWorkers << '\n';
  }
  return aStream;
}
