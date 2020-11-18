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

#include <proxygen/httpserver/samples/hq/HQParams.h>
#include <set>
#include <string>
#include <thread>

namespace uiiit {
namespace edge {

/**
 * Generic edge server providing a multi-threaded gRPC server interface for the
 * processing of lambda functions.
 */
class EdgeServerQuic
{

 public:
  NONCOPYABLE_NONMOVABLE(EdgeServerQuic);

  //! Create an edge server with a given number of threads.
  explicit EdgeServerQuic(const std::string& aServerEndpoint,
                          const size_t       aNumThreads);

  virtual ~EdgeServerQuic();

  //! Start the server. No more configuration allowed after this call.
  void run();

  //! Wait until termination of the server.
  void wait();

 protected:
  /**
   * \return the set of the identifiers of the threads that have been
   * spawned during the call to run(). The cardinality of this set
   * if equal to the number of threads specified in the ctor. If
   * run() has not (yet) been called, then an empty set is returned.
   */
  std::set<std::thread::id> threadIds() const;

 private:
  //****************** private members HQServer.h
  quic::samples::HQParams theParams;
  //******************

  //! Execute initialization logic immediately after run().
  virtual void init() {
  }

  //! Perform actual processing of a lambda request.
  virtual std::string process(const std::string& aReq) = 0;

 protected:
  const std::string theServerEndpoint;
  const size_t      theNumThreads;
}; // end class EdgeServer

} // namespace edge
} // namespace uiiit
