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

#include "Edge/edgeclientinterface.h"
#include "Edge/edgemessages.h"
#include "Support/macros.h"

#include <map>
#include <utility>

namespace uiiit {

namespace etsimec {
class AppContextManager;
}

namespace edge {

class EdgeClientGrpc;

/**
 * An edge client that uses an ETSI application context manager to select the
 * edge node to which the lambda requests are sent.
 *
 * This is done by polling the AppContextManager before issuing every lambda
 * request.
 *
 * Upon execution of a lambda an application context is created on the ETSI UE
 * application LCM proxy, if not already present, via the app context manager.
 * The UE application to use is selected from the catalog exposed by the LCM
 * proxy, which is refreshed at every context creation:
 * - if a matching UE application is not found, then the execution of the
 *   lambda fails;
 * - if there are multiple options, then the first occurence is picked, without
 * looking to the appCharcs structure within the AppInfo's.
 */
class EtsiEdgeClient final : public EdgeClientInterface
{
  NONCOPYABLE_NONMOVABLE(EtsiEdgeClient);

  struct Desc final {
    NONCOPYABLE_NONMOVABLE(Desc);

    explicit Desc(const std::string&                aUeAppId,
                  const std::string&                aReferenceUri,
                  std::unique_ptr<EdgeClientGrpc>&& aClient);

    Desc(Desc&& aOther);

    ~Desc();

    //! Initized upon context creation.
    const std::string theUeAppId;

    //! Updated according to the app context manager.
    std::string theReferenceUri;

    //! Edge client to actually perform execution of lambda functions.
    std::unique_ptr<EdgeClientGrpc> theClient;
  };

 public:
  /**
   * \param aAppContextManager The ETSI application context manager.
   * \param aSecure If true use SSL/TLS authentication towards the edge server.
   */
  explicit EtsiEdgeClient(etsimec::AppContextManager& aAppContextManager,
                          const bool                  aSecure);

  ~EtsiEdgeClient();

  /*
   * \param aDry if true do not actually execute the lambda
   */
  LambdaResponse RunLambda(const LambdaRequest& aReq, const bool aDry) override;

  //! \return the UE app ID of a given lambda; return empty string if nocontext.
  std::string ueAppId(const std::string& aLambdaName);

 private:
  /**
   * If a context exists then the app context manager is queried to check if
   * the reference URI is still the same: if not, then a new edge client is
   * created to contact the given reference URI, otherwise the existing client
   * is used.
   *
   * Otherwise, the method contacts the ETSI UE application LCM proxy
   * to retrieve the list of applications, find the missing details for the
   * AppInfo structure, then use the latter to create a context.
   *
   * \param aLambda The lambda name.
   *
   * \return the edge client to be used.
   *
   * \throw std::runtime_error if there is no suitable application in the UE
   * application LCM proxy or the context creation procedure failed.
   */
  EdgeClientGrpc& find(const std::string& aLambda);

 private:
  etsimec::AppContextManager& theAppContextManager;
  const bool                  theSecure;

  // key: lambda name
  // value: descriptor of the lambda
  std::unordered_map<std::string, Desc> theClients;
}; // end class EtsiEdgeClient

} // namespace edge
} // namespace uiiit
