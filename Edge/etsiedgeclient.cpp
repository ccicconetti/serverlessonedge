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

#include "etsiedgeclient.h"

#include "Edge/edgeclient.h"
#include "EtsiMec/appcontextmanager.h"
#include "EtsiMec/applistclient.h"

#include "glog/logging.h"

#include <algorithm>

namespace uiiit {
namespace edge {

EtsiEdgeClient::Desc::Desc(const std::string&            aUeAppId,
                           const std::string&            aReferenceUri,
                           std::unique_ptr<EdgeClient>&& aClient)
    : theUeAppId(aUeAppId)
    , theReferenceUri(aReferenceUri)
    , theClient(std::forward<std::unique_ptr<EdgeClient>>(aClient)) {
}

EtsiEdgeClient::Desc::Desc(Desc&& aOther)
    : theUeAppId(aOther.theUeAppId)
    , theReferenceUri(std::move(aOther.theReferenceUri))
    , theClient(std::move(aOther.theClient)) {
} 

EtsiEdgeClient::Desc::~Desc() {
}

EtsiEdgeClient::EtsiEdgeClient(etsimec::AppContextManager& aAppContextManager)
    : EdgeClientInterface()
    , theAppContextManager(aAppContextManager)
    , theClients() {
}

EtsiEdgeClient::~EtsiEdgeClient() {
  // nihil
}

LambdaResponse EtsiEdgeClient::RunLambda(const LambdaRequest& aReq,
                                         const bool           aDry) {
  const auto& myLambda = aReq.theName;
  if (myLambda.empty()) {
    throw std::runtime_error("Invalid empty lambda name");
  }

  auto& myClient = find(myLambda);

  return myClient.RunLambda(aReq, aDry);
}

std::string EtsiEdgeClient::ueAppId(const std::string& aLambdaName) {
  auto it = theClients.find(aLambdaName);

  if (it == theClients.end()) {
    return std::string();
  }

  assert(not it->second.theUeAppId.empty());
  return it->second.theUeAppId;
}

EdgeClient& EtsiEdgeClient::find(const std::string& aLambda) {
  auto it = theClients.find(aLambda);

  // create a context if it does not exist already
  if (it == theClients.end()) {
    LOG(INFO) << "Retrieving list of UE applictions from LCM proxy";
    // retrieve list from LCM proxy
    const auto myApplications =
        etsimec::AppListClient(theAppContextManager.proxyUri())();

    const auto jt = std::find_if(myApplications.begin(),
                                 myApplications.end(),
                                 [&aLambda](const etsimec::AppInfo& aAppInfo) {
                                   return aLambda == aAppInfo.appName();
                                 });

    if (jt == myApplications.end()) {
      throw std::runtime_error("UE application " + aLambda +
                               " not found in the LCM proxy");
    }

    // create the context, with an empty appPackageSource
    const auto myContext =
        theAppContextManager.contextCreate(jt->makeAppContextRequest(""));

    LOG(INFO) << "Creating context for " << aLambda << " with AppInfo: " << *jt;
    auto myEmplaceRet = theClients.emplace(
        aLambda,
        Desc(myContext.first,
             myContext.second,
             std::make_unique<EdgeClient>(myContext.second)));
    assert(myEmplaceRet.second);
    it = myEmplaceRet.first; // overrides outer iterator
  }

  assert(it != theClients.end());

  // check if the UE application has been assigned a new reference URI
  const auto myNewReferenceUri =
      theAppContextManager.referenceUri(it->second.theUeAppId);
  if (it->second.theReferenceUri != myNewReferenceUri) {
    // drop the existing edge client and create a new one that directs
    // lambda functions to the latest reference URI
    LOG(INFO) << "Reference URI for " << aLambda << " updated from "
              << it->second.theReferenceUri << " to " << myNewReferenceUri;
    it->second.theReferenceUri = myNewReferenceUri;
    it->second.theClient.reset(new EdgeClient(myNewReferenceUri));
  }

  assert(it->second.theClient.get() != nullptr);

  return *it->second.theClient;
}

} // namespace uiiit
} // namespace uiiit
