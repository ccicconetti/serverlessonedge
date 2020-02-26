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

#include "processloadserver.h"

namespace uiiit {
namespace edge {

ProcessLoadServer::ProcessLoadServer(const std::string& aServerEndpoint,
                                     const std::string& aCpuName,
                                     const double       aPeriod)
    : theServer(aServerEndpoint)
    , thePeriod(
          std::chrono::milliseconds(static_cast<long>(0.5 + aPeriod * 1e3)))
    , theStatus(RUNNING)
    , theProcessLoad()
    , theCurLoad()
    , theUsrLoadValue(theCurLoad.insert({aCpuName + "-usr", 0.0}).first)
    , theSysLoadValue(theCurLoad.insert({aCpuName + "-sys", 0.0}).first)
    , theMutex()
    , theCv()
    , theLoad10Wnd(10)
    , theLoad30Wnd(30)
    , theThread([this]() {
      while (run())
        ;
    }) {
  assert(theUsrLoadValue != theCurLoad.end());
  assert(theSysLoadValue != theCurLoad.end());
  theServer.run(false); // non-blocking
}

ProcessLoadServer::~ProcessLoadServer() {
  std::unique_lock<std::mutex> myLock(theMutex);
  theStatus = TERMINATING;
  theCv.notify_one();
  theCv.wait(myLock, [this]() { return theStatus == TERMINATED; });
  assert(theThread.joinable());
  theThread.join();
}

std::array<double, 3> ProcessLoadServer::lastUtils() const noexcept {
  const std::lock_guard<std::mutex> myLock(theMutex);
  try {
    return {
        {theLoad30Wnd.last(), theLoad10Wnd.average(), theLoad30Wnd.average()}};
  } catch (...) {
    // it means the windows are empty
  }
  return {{0.0, 0.0, 0.0}};
}

bool ProcessLoadServer::run() {
  std::unique_lock<std::mutex> myLock(theMutex);
  theCv.wait_for(myLock, thePeriod, [this]() { return theStatus != RUNNING; });

  if (theStatus == TERMINATING) {
    theStatus = TERMINATED;
    theCv.notify_one();
    return false;
  }

  assert(theStatus == RUNNING);
  const auto myLoad       = theProcessLoad();
  theUsrLoadValue->second = myLoad.first;
  theSysLoadValue->second = myLoad.second;
  theServer.add(theCurLoad);

  // add the current user load to the moving average window
  theLoad10Wnd.add(myLoad.first);
  theLoad30Wnd.add(myLoad.first);

  return true;
}

} // namespace edge
} // end namespace uiiit
