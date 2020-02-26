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

#include "Support/macros.h"
#include "edgeclient.h"
#include "edgemessages.h"

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <condition_variable>
#include <utility>

namespace uiiit {
namespace edge {

/**
 * A thread-safe pool of edge clients.
 */
class EdgeClientPool
{
  struct Descriptor {
    explicit Descriptor()
        : theFree()
        , theBusy(0)
        , theAvailableCond() {
    }

    std::list<std::unique_ptr<EdgeClient>> theFree;
    size_t                                 theBusy;
    std::condition_variable                theAvailableCond;
  };

 public:
  NONCOPYABLE_NONMOVABLE(EdgeClientPool);

  /**
   * Create a pool with no clients that can host up to a max.
   *
   * \param aMaxClients The maximum number of clients per destination. 0 means
   * ulimited.
   */
  explicit EdgeClientPool(const size_t aMaxClients);

  /**
   * Execute a lambda on a given edge computer identified by its end-point.
   *
   * If a client already exists in the pool then we use it, otherwise a new
   * instance is created and added to the pool.
   *
   * \param aDestination The edge computer end-point.
   * \param aReq The lambda request.
   * \param aDry If true do not actually execute the lambda function.
   *
   * \return the lambda response and the execution time.
   */
  std::pair<LambdaResponse, double> operator()(const std::string& aDestination,
                                               const LambdaRequest& aReq,
                                               const bool           aDry);

 private:
  std::unique_ptr<EdgeClient> getClient(const std::string& aDestination);

  void releaseClient(const std::string&            aDestination,
                     std::unique_ptr<EdgeClient>&& aClient);

  void debugPrintPool();

 private:
  const size_t theMaxClients;

  mutable std::mutex                theMutex;
  std::map<std::string, Descriptor> thePool;
};

} // namespace edge
} // namespace uiiit
