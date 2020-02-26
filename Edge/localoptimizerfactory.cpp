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

#include "localoptimizerfactory.h"

#include "Support/conf.h"
#include "localoptimizerasync.h"
#include "localoptimizernone.h"
#include "localoptimizertrivial.h"

namespace uiiit {
namespace edge {

std::shared_ptr<LocalOptimizer>
LocalOptimizerFactory::make(ForwardingTable&     aTable,
                            const support::Conf& aConf) {
  std::unique_ptr<LocalOptimizer> myRet;

  const auto myType = aConf("type");

  if (myType == "none") {
    myRet.reset(new LocalOptimizerNone(aTable));

  } else if (myType == "trivial") {
    myRet.reset(new LocalOptimizerTrivial(
        aTable,
        aConf.getDouble("period"),
        LocalOptimizerTrivial::statFromString(aConf("stat"))));

  } else if (myType == "async") {
    myRet.reset(new LocalOptimizerAsync(aTable, aConf.getDouble("alpha")));

  } else {
    throw std::runtime_error("Invalid local optimizer type: " + myType);
  }

  return myRet;
}

} // namespace edge
} // namespace uiiit
