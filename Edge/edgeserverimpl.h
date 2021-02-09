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

#include "Support/macros.h"
#include "edgeserver.h"

#include <cassert>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>

namespace uiiit {
namespace edge {

struct LambdaResponse;

/**
 * Generic edge server base class for the
 * processing of lambda functions.
 */
class EdgeServerImpl
{

 public:
  NONCOPYABLE_NONMOVABLE(EdgeServerImpl);

  //! Create a base class with a reference to an edge server.
  explicit EdgeServerImpl(EdgeServer& aEdgeServer);

  virtual ~EdgeServerImpl();

  //! Start the server. No more configuration allowed after this call.
  virtual void run() = 0;

  //! Wait until termination of the server.
  virtual void wait() = 0;

  //  private:
  //   //! Thread execution body.
  //   void handle();

  //! Perform actual processing of a lambda request.
  virtual rpc::LambdaResponse process(const rpc::LambdaRequest& aReq) = 0;

 protected:
  EdgeServer& theEdgeServer;

}; // end class EdgeServer

} // end namespace edge
} // end namespace uiiit
