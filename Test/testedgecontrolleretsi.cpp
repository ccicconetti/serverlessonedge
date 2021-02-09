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

#include "gtest/gtest.h"

#include "Edge/edgecontrolleretsi.h"
#include "Edge/edgecontrollermessages.h"
#include "EtsiMec/trivialueapplcmproxy.h"
#include "Rest/client.h"
#include "Support/wait.h"

#include <glog/logging.h>

#include <map>
#include <thread>

namespace uiiit {
namespace edge {

struct TestEdgeControllerEtsi : public ::testing::Test {
  TestEdgeControllerEtsi()
      : theProxyUri("http://localhost:10000") {
  }

  void countEq(rest::Client& aClient, const size_t aExpected) {
    const auto myRet = aClient.get("/app_list");
    ASSERT_EQ(web::http::status_codes::OK, myRet.first);
    ASSERT_TRUE(myRet.second.has_object_field("ApplicationList"));
    ASSERT_TRUE(myRet.second.at("ApplicationList").has_array_field("appInfo"));
    ASSERT_EQ(
        aExpected,
        myRet.second.at("ApplicationList").at("appInfo").as_array().size());
  }

  const std::string theProxyUri;
};

TEST_F(TestEdgeControllerEtsi, test_edgecontrolleretsi) {
  etsimec::TrivialUeAppLcmProxy myProxy(theProxyUri);
  myProxy.start();

  // bind LCM proxy to the edge controller
  EdgeControllerEtsi myEdgeControllerEtsi(myProxy);

  rest::Client myClient(theProxyUri + "/" + myProxy.apiName() + "/" +
                        myProxy.apiVersion());
  countEq(myClient, 0);

  // create some containers
  ContainerList myContainer1{
      {{ContainerList::Container{"cont0", "cpu0", std::string("lambda0"), 2}}},
  };
  ContainerList myContainer2{
      {{ContainerList::Container{"cont0", "cpu0", std::string("lambda0"), 2}},
       {ContainerList::Container{"cont1", "cpu0", std::string("lambda1"), 2}},
       {ContainerList::Container{"cont2", "cpu0", std::string("lambda2"), 2}}},
  };

  // publish some lamdbas using the controller
  myEdgeControllerEtsi.announceComputer("host0:6473", myContainer1);
  countEq(myClient, 1);

  myEdgeControllerEtsi.announceComputer("host1:6473", myContainer2);
  countEq(myClient, 3);

  // check that they appear/disappear as needed
  myEdgeControllerEtsi.removeComputer("host1:6473");
  countEq(myClient, 1);

  myEdgeControllerEtsi.removeComputer("host0:6473");
  countEq(myClient, 0);
}

} // namespace edge
} // namespace uiiit
