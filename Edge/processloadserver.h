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

#pragma once

#include "Edge/edgecomputerserver.h"
#include "Support/movingavg.h"
#include "Support/process.h"

#include <array>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace uiiit {
namespace edge {

/**
 * Starts a background thread that periodically emits the current user/system
 * load.
 */
class ProcessLoadServer final
{
  enum Status { RUNNING, TERMINATING, TERMINATED };

 public:
  /**
   * \param aServerEndpoint The end-point of the gRPC server.
   * \param aCpuName The CPU name to use when notifying gRPC clients.
   * \param aPeriod The polling period, in fractional seconds.
   */
  explicit ProcessLoadServer(const std::string& aServerEndpoint,
                             const std::string& aCpuName,
                             const double       aPeriod);

  ~ProcessLoadServer();

  //! \return the last 1-second, 10-second, and 30-second user CPU loads.
  std::array<double, 3> lastUtils() const noexcept;

 private:
  bool run();

 private:
  using LoadMap = std::map<std::string, double>;

  EdgeComputerServer         theServer;
  std::chrono::milliseconds  thePeriod;
  Status                     theStatus;
  support::ProcessLoad       theProcessLoad;
  LoadMap                    theCurLoad;
  LoadMap::iterator          theUsrLoadValue;
  LoadMap::iterator          theSysLoadValue;
  mutable std::mutex         theMutex;
  std::condition_variable    theCv;
  support::MovingAvg<double> theLoad10Wnd;
  support::MovingAvg<double> theLoad30Wnd;
  std::thread                theThread;
};

} // end namespace edge
} // end namespace uiiit
