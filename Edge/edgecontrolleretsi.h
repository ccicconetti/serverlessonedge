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

#include "Edge/edgecontroller.h"
#include "EtsiMec/appinfo.h"
#include "EtsiMec/ueapplcmproxy.h"

namespace uiiit {
namespace edge {

/**
 * Convey notifications from an EdgeController to an ETSI UE application LCM
 * proxy.
 *
 * Fill the AppInfo structure as follows:
 * - name: lambda name
 * - provider: ServerlessOnEdge
 * - version: 1.0
 * - description: <empty>
 * - application characteristics: <default>
 */
class EdgeControllerEtsi final : public EdgeController
{
 public:
  explicit EdgeControllerEtsi(etsimec::UeAppLcmProxy& aUeAppLcmProxy);

 private:
  void addComputer(const std::string&, const std::list<std::string>&) override {
  }

  void delComputer(const std::string&, const std::list<std::string>&) override {
  }

  void addLambda(const std::string& aLambda) override;

  void delLambda(const std::string& aLambda) override;

  void privateAnnounceComputer(const std::string&,
                               const ContainerList&) override {
  }

  void privateAnnounceRouter(const std::string&, const std::string&) override {
  }

  void privateRemoveComputer(const std::string&,
                             const std::list<std::string>&) override {
  }
  void privateRemoveRouter(const std::string&) override {
  }

  static etsimec::AppInfo make(const std::string& aLambda);

 private:
  etsimec::UeAppLcmProxy& theUeAppLcmProxy;
};

} // namespace edge
} // namespace uiiit
