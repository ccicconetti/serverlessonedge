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

#include "Support/stat.h"
#include "localoptimizerperiodic.h"

#include <map>
#include <mutex>

namespace uiiit {
namespace edge {

class LocalOptimizerFactory;

class LocalOptimizerTrivial final : public LocalOptimizerPeriodic
{
  friend class LocalOptimizerFactory;

  // key:   lambda
  // value: map of key:   destination
  //               value: latency stats
  using Latencies =
      std::map<std::string, std::map<std::string, support::SummaryStat>>;

 public:
  enum class Stat {
    MEAN = 0,
    MIN  = 1,
    MAX  = 2,
  };

  void operator()(const rpc::LambdaRequest& aReq,
                  const std::string&        aDestination,
                  const double              aTime) override;

  static Stat        statFromString(const std::string& aStat);
  static std::string toString(const Stat aStat);

 private:
  explicit LocalOptimizerTrivial(ForwardingTable& aForwardingTable,
                                 const double     aPeriod,
                                 const Stat       aStat);

  //! Update the local table and reset all statistics.
  void update() override;

  //! Balance a single entry.
  void balance(const std::string&            aLambda,
               const Latencies::mapped_type& aEntry);

 private:
  const Stat theStat;
  std::mutex theMutex;
  Latencies  theLatencies;
};

} // namespace edge
} // namespace uiiit
