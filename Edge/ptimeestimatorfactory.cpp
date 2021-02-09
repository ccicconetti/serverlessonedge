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

#include "ptimeestimatorfactory.h"

#include "Edge/ptimeestimatordelay.h"
#include "Edge/ptimeestimatorprobe.h"
#include "Edge/ptimeestimatorrtt.h"
#include "Edge/ptimeestimatorutil.h"
#include "Support/conf.h"

namespace uiiit {
namespace edge {

std::shared_ptr<PtimeEstimator>
PtimeEstimatorFactory::make(const support::Conf& aConf) {
  std::unique_ptr<PtimeEstimator> myRet;

  const auto myType = aConf("type");

  const auto it = defaultConfs().find(myType);
  if (it == defaultConfs().end()) {
    throw std::runtime_error("Invalid processing time estimator type: " +
                             myType);
  }

  try {
    if (myType == "rtt") {
      myRet.reset(new PtimeEstimatorRtt(aConf.getUint("window-size"),
                                        aConf.getDouble("stale-period")));
    } else if (myType == "util") {
      myRet.reset(new PtimeEstimatorUtil(aConf.getUint("rtt-window-size"),
                                         aConf.getDouble("rtt-stale-period"),
                                         aConf.getDouble("util-load-timeout"),
                                         aConf.getDouble("util-window-size"),
                                         aConf("output")));
    } else if (myType == "delay") {
      myRet.reset(new PtimeEstimatorDelay(aConf.getDouble("util-load-timeout"),
                                          aConf.getDouble("util-window-size"),
                                          aConf("output")));
    } else if (myType == "probe") {
      myRet.reset(new PtimeEstimatorProbe(aConf.getUint("max-clients"),
                                          aConf("output")));
    } else {
      assert(false);
    }
  } catch (const support::ConfException& aErr) {
    throw std::runtime_error("Invalid configuration for type '" + myType +
                             "', use something like: " + it->second);
  }

  return myRet;
}

std::set<std::string> PtimeEstimatorFactory::types() {
  std::set<std::string> ret;
  for (const auto& myPair : defaultConfs()) {
    ret.emplace(myPair.first);
  }
  return ret;
}

std::string PtimeEstimatorFactory::defaultConf(const std::string& aType) {
  const auto it = defaultConfs().find(aType);
  if (it == defaultConfs().end()) {
    throw std::runtime_error("Invalid processing time estimator type: " +
                             aType);
  }
  return it->second;
}

const std::map<std::string, std::string>&
PtimeEstimatorFactory::defaultConfs() {
  static const std::map<std::string, std::string> theDefaultConfs({
      {"rtt", "type=rtt,window-size=50,stale-period=10"},
      {"delay",
       "type=delay,"
       "util-load-timeout=10,util-window-size=50,"
       "output=out.dat"},
      {"util",
       "type=util,rtt-window-size=50,rtt-stale-period=10,"
       "util-load-timeout=10,util-window-size=50,"
       "output=out.dat"},
      {"probe", "type=probe,max-clients=10,output=out.dat"},
  });
  return theDefaultConfs;
}

} // namespace edge
} // namespace uiiit
