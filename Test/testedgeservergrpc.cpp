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

#include "Edge/edgeclientgrpc.h"
#include "Edge/edgeserver.h"
#include "Edge/edgeservergrpc.h"

#include "gtest/gtest.h"

#include <boost/filesystem.hpp>

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <thread>

namespace uiiit {
namespace edge {

class TrivialEdgeServer final : public EdgeServer
{
 public:
  explicit TrivialEdgeServer(const std::string& aEndpoint,
                             const std::string& aLambda,
                             const std::string& aThrowingLambda)
      : EdgeServer(aEndpoint)
      , theLambda(aLambda)
      , theThrowingLambda(aThrowingLambda) {
    // noop
  }

  // convert input to uppercase
  // throw if the lambda's name does not match
  rpc::LambdaResponse process(const rpc::LambdaRequest& aReq) override {
    rpc::LambdaResponse ret;

    if (aReq.name() == theThrowingLambda) {
      throw std::runtime_error("exception raised");
    } else if (aReq.name() != theLambda) {
      ret.set_retcode("invalid lambda: " + aReq.name());

    } else {
      ret.set_retcode("OK");
      std::string myResponse(aReq.input().size(), '\0');
      for (size_t i = 0; i < aReq.input().size(); i++) {
        myResponse[i] = std::toupper(aReq.input()[i]);
      }
      ret.set_output(myResponse);
    }
    return ret;
  }

 private:
  const std::string theLambda;
  const std::string theThrowingLambda;
};

struct TestEdgeServerGrpc : public ::testing::Test {
  TestEdgeServerGrpc()
      : theEndpoint("localhost:6666") {
    // noop
  }

  void SetUp() {
    boost::filesystem::remove_all("to_remove");
    boost::filesystem::create_directories("to_remove");

    std::ofstream myCaCrt("to_remove/ca.crt");
    myCaCrt
        << "-----BEGIN CERTIFICATE-----\n"
           "MIIFpzCCA4+gAwIBAgIULoxohuKnwcH7986H4QT/oq7hiZkwDQYJKoZIhvcNAQEL\n"
           "BQAwYzELMAkGA1UEBhMCU1AxDjAMBgNVBAgMBVNwYWluMRQwEgYDVQQHDAtWYWxk\n"
           "ZXBlbmlhczENMAsGA1UECgwEVGVzdDENMAsGA1UECwwEVGVzdDEQMA4GA1UEAwwH\n"
           "Um9vdCBDQTAeFw0yMjA5MTkxMDE1MzZaFw0yMzA5MTkxMDE1MzZaMGMxCzAJBgNV\n"
           "BAYTAlNQMQ4wDAYDVQQIDAVTcGFpbjEUMBIGA1UEBwwLVmFsZGVwZW5pYXMxDTAL\n"
           "BgNVBAoMBFRlc3QxDTALBgNVBAsMBFRlc3QxEDAOBgNVBAMMB1Jvb3QgQ0EwggIi\n"
           "MA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQDhrI+eB4Q11UrlIe+KJbYFJ5Hp\n"
           "gFbxp3xDpMSW43qV0OgqtFBxAuct9A5jSeo2Y3VLO9s3CGte+hBx5Hkse6+5p0Up\n"
           "ICdk0DfmNh2B9qYstqjfie+zX6CQ7sSIri4/L2UDk2qJg+inJeb1Y96QSMh2jubn\n"
           "Lrb2AgFoTyWYsGJjrRVc5C3z5BySQRtai56nSwIpTgsdzRIPbnEdFWl4eGP50x3o\n"
           "Z7Gyb6y0Al+jrzVNOe/FQglUIxA2HEWRSYJPRuWb2Y9BHTV3GSb4t7iJGVZ2Szod\n"
           "23nuFZliYL+rzc8an13DTsb2dt/j+ixSMnbAYmvaKI6SxslzEPUtPNmvHSkL8dDL\n"
           "hepAZ/3HhCfu8qmrqBX1WmI44s1A/Y21IeBCxIe7HG96k3eJzihsld+F3KMefuDw\n"
           "PzqcHEHEcLO96xfnQ2fMFvDHasMFVprOKt5G0kI8QH9nkHz63yep2NiOL4qdYXqe\n"
           "LREtHgxvlkwpyJR3C+bNA3Ig2y062+c4MdGmFPT+uLczBzFdziN+5Dvw2TyerUQI\n"
           "Mqc0FzwDrJdkHtOuplRx0tie2JNb0q4L1TYyl62IKGfKC3mBug+XpRNGmKEVj/l4\n"
           "9ezleM9SYbNM1FmCfQ1pjbFji5n7Pxb+Sjqex8SUioH9P8tntf7tPj2oHBn868YH\n"
           "IUZobDx/xnDpd+4JYwIDAQABo1MwUTAdBgNVHQ4EFgQUJENrPOpc2Vm6TkHLEyRy\n"
           "iap+FGowHwYDVR0jBBgwFoAUJENrPOpc2Vm6TkHLEyRyiap+FGowDwYDVR0TAQH/\n"
           "BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAgEAMUnlrrLDGGeRaXX35uADYJWCaNpn\n"
           "hNf83X0/a6QYmPNj/fl5iGBf0MDk997dfEtIm5+7uqFMKC63GfIA6kqtcrzCOdXO\n"
           "8tkJjzP3mW9rOSPVAvRlaeqEcrn+DVm1+dxhbaxMwDH87EvZ6A+Jb/ZbTYayGkcr\n"
           "W4GKJLRYsi0jPxLoEhCt6vGMbV1964gjVHM2/izw6C2QjtDKe9wstoxehc/N95NO\n"
           "K20QTrdepPsOHtR+VgcGotOQEYbBQOMdUKO4ETRx25BEs89VfLBoE/LgViplxazb\n"
           "nK4hh9K+tvApCTO05V6zynTImUBEIjq1NIapwNNuHIZSqnLVa6f1DAxlzaNnOfao\n"
           "hAdPMLnGMRdPKs1oLECDvlfpMo9UtABZKLvb7hwlDrcF7wSdvbDXVoIXiBgQQ+FN\n"
           "HheKZlmMEGjeXbrYhvsZtAwRlyMnIwI0NMRxWXG+5n9C5XI9vC7G+NrvwOYOUyqE\n"
           "cMudFsA9u4HlAOL7DFrvTFUfC7buu+rg9HgAEDF/xBbp33bCofL9aFVEMxgI96R4\n"
           "vXkwLpJhYhPJVkRhcUD2mR0XbeUs9qGmusu5mLhKSf6A36OebGfWwPmp07DuHPe3\n"
           "0G1jwbF5hYclnVUaTtz9TmYvqp78GVSuAV+z1Q0YGQKgXVv1xidkHTOf0aQ9cid3\n"
           "mPZ8CZlkv1QUeYI=\n"
           "-----END CERTIFICATE-----";

    std::ofstream myServerCrt("to_remove/server.crt");
    myServerCrt
        << "-----BEGIN CERTIFICATE-----\n"
           "MIIFPjCCAyYCAQEwDQYJKoZIhvcNAQELBQAwYzELMAkGA1UEBhMCU1AxDjAMBgNV\n"
           "BAgMBVNwYWluMRQwEgYDVQQHDAtWYWxkZXBlbmlhczENMAsGA1UECgwEVGVzdDEN\n"
           "MAsGA1UECwwEVGVzdDEQMA4GA1UEAwwHUm9vdCBDQTAeFw0yMjA5MTkxMDE1NDBa\n"
           "Fw0yMzA5MTkxMDE1NDBaMGcxCzAJBgNVBAYTAlNQMQ4wDAYDVQQIDAVTcGFpbjEU\n"
           "MBIGA1UEBwwLVmFsZGVwZW5pYXMxDTALBgNVBAoMBFRlc3QxDzANBgNVBAsMBlNl\n"
           "cnZlcjESMBAGA1UEAwwJbG9jYWxob3N0MIICIjANBgkqhkiG9w0BAQEFAAOCAg8A\n"
           "MIICCgKCAgEAtOXAUwTa3t/9sBsJ8VGkghdNFeOn+Mt3flt9+sx+QmQ6qebESfab\n"
           "c9PHzHcEF+aci/kNECWVyS27EcDqL3ph0oQCxkznQXMHuFhyjX8NbwIsV679uD/h\n"
           "JuRKcNF+4ovm6jFe0dGV7CA4iOkdcoV1m2v1h+SbNIbLxALI42FDyeBRhwfwhVMG\n"
           "VSzCy9KTL0sMPZmuj/tSzx39cUAwCGdN1ryloKCCFQeWjXybX/rVrM5vvXVyHA6c\n"
           "or8IF7T7DS0eaVY5HRfNOk0ze6JHhg9OfoeYrmNt8chHvq9ypSiDjZtZHadeML0/\n"
           "xLFYN1Sy690bEK//pjHNzW/oe6QISbcroalFcMPZlV1v2A3zs6gdrx6pF/iN+gFd\n"
           "c/KOlyYcUURvGsLz2z/Sg5tfYrR2Wpw5sXj1L0JAYX1xdNP02GpG4KA8B+evUHUy\n"
           "BGzs5pqDCdcE2KmildKpGAS0bYPtJf7RFYWEBxnWeNR/OOOeNAhrk1eMOdJ59R9I\n"
           "1/TUAIACyII473Z0vSBtOG3FmXJCKuJXZwHV1FruaYQk3oXxqOzbAjj1kAjuAfYZ\n"
           "ukhIHV8kG/gZWhDySY+/2F75/+/0cIrQhH0aTkrT/eurZ3GbVHPWVug/r+/chGLd\n"
           "q2EX3tBwMN+xYhmtfII4iWS4bZmXLkx/RSPeAQ4PK4SCBmgP47Cg90sCAwEAATAN\n"
           "BgkqhkiG9w0BAQsFAAOCAgEA0PVutbzbTxOIMoKDlxy6ZQUA+cdwLuYG13IpqFBp\n"
           "HQlWSuHxL3aTXmW5D3lgHI0qDt2r3oNlqxI1FchvV2r1tyLji1flHVz771GfHOyc\n"
           "hECjWOLkRRxst0zTaz1T+CL6ALhMQexkDWCTfIh8dnwB/MboMav3wYY2gbaUJiMb\n"
           "FdIyRb1lXctcJnOO4xrEPtk3h6ACmbIHkxE2VXvxoBDKJwEm0hIxViT1QWmO363j\n"
           "ldGSOZ3VkPDStguREx9oAGCQEXrsmHSxbxXdZbIxBtm14x+VPMVV41gzngOM5PkY\n"
           "ioTqCkuC78FBORZnoZfZlEqt6m2mwbB5SvGyXH071yt9R7X8mDfFbp3vrG8H/pDE\n"
           "85vBLMCfMbAaiL4BxHCBe6kcBaiBcnNFp7BQzIQ3QgAPeJ5xxEn24O//TqhrHVZX\n"
           "DziCe6Sk1hhqPO/GM7nXhGQXMP5fUjVRH0Tbz2t0SZf5x/L2DHRzw/RElxcAZ9m9\n"
           "0GG6lpSm/5cp7aU6x4eN+SdadQm6UpRLKJvgUhD0wwKp2276RZ1bRDzLUD3emHYc\n"
           "rxOs1o7DCwm2gYIQvf0kDJ1AqyYlYPlIHGEC7Yx3P9+K2RM1VDXFyxEUWi+vOiAR\n"
           "fX6TMu6FVl5ZcH11iGbe9Fa3pstIshhmmCV8kTlCNTpb2T5qtPBW7jbmqYrqBTSB\n"
           "2U0=\n"
           "-----END CERTIFICATE-----";

    std::ofstream myServerKey("to_remove/server.key");
    myServerKey
        << "-----BEGIN RSA PRIVATE KEY-----\n"
           "MIIJKgIBAAKCAgEAtOXAUwTa3t/9sBsJ8VGkghdNFeOn+Mt3flt9+sx+QmQ6qebE\n"
           "Sfabc9PHzHcEF+aci/kNECWVyS27EcDqL3ph0oQCxkznQXMHuFhyjX8NbwIsV679\n"
           "uD/hJuRKcNF+4ovm6jFe0dGV7CA4iOkdcoV1m2v1h+SbNIbLxALI42FDyeBRhwfw\n"
           "hVMGVSzCy9KTL0sMPZmuj/tSzx39cUAwCGdN1ryloKCCFQeWjXybX/rVrM5vvXVy\n"
           "HA6cor8IF7T7DS0eaVY5HRfNOk0ze6JHhg9OfoeYrmNt8chHvq9ypSiDjZtZHade\n"
           "ML0/xLFYN1Sy690bEK//pjHNzW/oe6QISbcroalFcMPZlV1v2A3zs6gdrx6pF/iN\n"
           "+gFdc/KOlyYcUURvGsLz2z/Sg5tfYrR2Wpw5sXj1L0JAYX1xdNP02GpG4KA8B+ev\n"
           "UHUyBGzs5pqDCdcE2KmildKpGAS0bYPtJf7RFYWEBxnWeNR/OOOeNAhrk1eMOdJ5\n"
           "9R9I1/TUAIACyII473Z0vSBtOG3FmXJCKuJXZwHV1FruaYQk3oXxqOzbAjj1kAju\n"
           "AfYZukhIHV8kG/gZWhDySY+/2F75/+/0cIrQhH0aTkrT/eurZ3GbVHPWVug/r+/c\n"
           "hGLdq2EX3tBwMN+xYhmtfII4iWS4bZmXLkx/RSPeAQ4PK4SCBmgP47Cg90sCAwEA\n"
           "AQKCAgEAsP0ta8yPPIrZEgmSc8pWc3XK1QTVnoWsVzO7EbwsOFcKUptXJ6qhs/Tc\n"
           "Qj+cAKqANi8pScgMQjZ9FkSynFtYBHlmoZAynwWl4waepbROd3Mf4XZ0BWZyqvOn\n"
           "pbRDOfJ5rl85j35I+isYRVQXnKnZRISfSSMxe3X7Pb8fv+C1S3ovKjdpa3is9H8D\n"
           "5BoWKap6wnZvpa3W+pA0HS8ZD8/LB7OxNXR1cY/oDGyDNKgdkaK1DyAu0ia1uEHB\n"
           "8DgBfYXMgHF9FerowAcyvN8srYyiVwfiKZVcx7gWAgp02ATIma8JxQqxDGzNTGvb\n"
           "jN9Q45aEWZJpRbjqy/qW8wmtljoh87d3UYIDJovxOrkrGgkQ/ChjUXganivq/8oZ\n"
           "xoPXekiZFz2MaQr3jDRsUGbYI5SFA7mF1XAWBf8+4N+WPQr9liDwH+9gLz/qVHwE\n"
           "NTb8Q6lJTYVaRmrDQ8/yFUlJdsu2/o3SosUZZ6dfctJ866Hb0FG86C6xQ61pLMN2\n"
           "bnhQELpOillgSSg0pbMJDd/WmaTl8UE9bI5uRiLOa4TU2uEzulIr7+0QqAO4PVlF\n"
           "qvsr9t5ubknvfRUliwCEI6vMQzTv2a5tF2YcDWQK5OODFWOoAiuxXMh6NX8Rq8zL\n"
           "kYCJZj4O4dyesU7lhIfZyTCfRC79WKPc6D89rc/3VzfuZZF+mFkCggEBAN3XiVOw\n"
           "OP+V1org7b7Xir5nvXMl7uUvymwvzND08NRzKzbrBD9pyVfAT7W1syL+7WduqUHA\n"
           "pbNzMe072/yihwtUoewLZmHg5wgeDh5YnLaYwqZxRYcxLSthkhjtCzeWyhQ5HMT8\n"
           "x0aV6xGV7/cNqudA0DsSNtAhUo8DmeBw4s55LCvxf+X/21TQxayXz7A8dg0HuMns\n"
           "empDL0Z71yFUS91dN6PyMrG2w+tc7/JRO8FVUJQ1TSQm9ZZCnNSB3D+OboreN6Ej\n"
           "7g1RJr3Mz9Xcrnc8j58Fa8Ic9WxQ7fxZgIfO/oJ6MGjQK7wVPvs/Asf4kPPruQYM\n"
           "wq0OA6NTycbaz/UCggEBANDASM8IFNyGZbaFUNLsliW8a+cOtQrrFtiGUjeeILCN\n"
           "dAiyPLpOqQJGXNS1uqmiunXhqtMcZaOK2uRc1GtEmTYVCFiTe6C8M+fktRdWo91t\n"
           "09S6suZ06KTtWRxgyA/Y5dPQSOrtCbl7BhF/UPr1m8Lr5LRK9+O+/GwQwmw7mrDC\n"
           "+23kJ2AmDxD36/eBXSJSG7sNNIglHgEM4KpbTH6sT6gHkQ4qWnNTJue3V6lw/Bh7\n"
           "WPzMCJMk2UIAosqDSqoVJ/ytBxdrIVvvGX+1lefLSEfLxufMgRv6qGFFQLN3eYh4\n"
           "0y6AB0p91/IxbtjzqCQbOoEnmaHmFTeWOK1vtSj8Yj8CggEBAISk5ZXGnq4j154+\n"
           "k6kyk+D5gouhONCM3MoHYr3mV0GQg1xY9bc894iikoah2DSqnSTlRArut6Uu0cF5\n"
           "szXBsGGT/yV2Q4duxHVUEzkN3tZHTMvcmqgVPV7c/lAtHDHm1Xl2FL+sLTMFXQKs\n"
           "1kiwQWn4lQldAK0933Fnw9Axb0vppAG6arEJZQm3sxpZw3MrE6MC5PjHjwKOnWvV\n"
           "jHJ/RsVbgXNj0+/yyf6nGUmYTNRywLV9kVx2dw2bUs61vw25ils6N6UAKMyJo1KF\n"
           "bhf/1cmxymZZBW+RhSV41nO2yGVr3T4C9YMWRbA2O+xaZr1O4M0YOw9k/dofsQkt\n"
           "hk6CcykCggEBALx8koFVU7jXJWoLsDHza5OBMZC8592G7ebuSbhscCnaX2Ymwcyb\n"
           "j+E89T8fI/9drq0X6W2bW26yxvihlS+SDiEWFqYb7OnyZY/CI32CPo2GSFnxhe2u\n"
           "rA2XPfnwrVQZzVNW4zA0fa5bldwgsFcZg94ZrzTX6EJcxYEyFMszenaWZ/4Onzsi\n"
           "wkHZCdg+l4CQ0PcxcjhoA1KBdS59J3HNVlNrGZ/HwEoXT660DlQxb38PiQchl7B8\n"
           "+iqtdlATAkzWs3dkKF4N3l4rY0O8CzHzjD0/k0bjkcRYrMa/iC9D9pmyN0TqVyr8\n"
           "vAq0ddkxEKYha88ImaDkpk29t0lq7xGl6XMCggEANGalfpIy98kdCnji+UvP5Un0\n"
           "N1dRrH56gMcrW3yFYSxha1TUSolMnauN1OGSY6RhOkRyRAy+Ztx/mJ38sTivrT8R\n"
           "S+tUoJ53ebBUrzxHjPVV776ql8YR6cGDa78ZdKtSkHiHbNCRWigMJFlhI+zwHCZC\n"
           "Lh3XBtgolOLJLXQg/riY1QXp883fFSBoLxU1GZv4undBwjT01IUL7D+nWHN1cP+q\n"
           "FeikaLz3T+dgR+CfTt1ZTteiqHBMhqLzeWl9244joidB/0u/4SAjQug6VBofmJES\n"
           "n3Oi+f6oQz/IwD9+P7UV89l78Z5zasYEIDZMCYs95BBY7S7wbD7JtangI8m2Zg==\n"
           "-----END RSA PRIVATE KEY-----";

    std::ofstream myClientCrt("to_remove/client.crt");
    myClientCrt
        << "-----BEGIN CERTIFICATE-----\n"
           "MIIFPjCCAyYCAQEwDQYJKoZIhvcNAQELBQAwYzELMAkGA1UEBhMCU1AxDjAMBgNV\n"
           "BAgMBVNwYWluMRQwEgYDVQQHDAtWYWxkZXBlbmlhczENMAsGA1UECgwEVGVzdDEN\n"
           "MAsGA1UECwwEVGVzdDEQMA4GA1UEAwwHUm9vdCBDQTAeFw0yMjA5MTkxMDE1NDFa\n"
           "Fw0yMzA5MTkxMDE1NDFaMGcxCzAJBgNVBAYTAlNQMQ4wDAYDVQQIDAVTcGFpbjEU\n"
           "MBIGA1UEBwwLVmFsZGVwZW5pYXMxDTALBgNVBAoMBFRlc3QxDzANBgNVBAsMBkNs\n"
           "aWVudDESMBAGA1UEAwwJbG9jYWxob3N0MIICIjANBgkqhkiG9w0BAQEFAAOCAg8A\n"
           "MIICCgKCAgEAt5DPL0Y+0N0U7Qgf+N6Rkf1XOQI0zTSAsxdP+mrx9r76PDqqG4I5\n"
           "FtA34ycrJe764wclZEMDukn6Py73U6AKOY7rivLnca/No+hKsZf8M6Jikk7R08wT\n"
           "E0FzePhWiWoDZZYueujfbwfLH0RoBlogIEUxiGr9EZqfDOnzxpdoQEttp/Hj19HX\n"
           "ufRsKEIJrgo+hSNyQTesGdK6yQmlDaPkmxz/86w+fUOoinjNR2LImwBuTR6XWzNq\n"
           "XcZvkCa+YMKVGKa1Bx80nrRbi+TWBJD6mxeU9aT0983fFWaUEdjhHRFGnggkUgmH\n"
           "eJq+tq7QxejtIrGLDDB3pLlDvVP38a1Jsi0aD6gArduEuFQ9pz7oF39yIu75Wmhu\n"
           "E58NBObaeGxMd1nObZjuYbIxGCg5BYF2cticCRb5gbqu/yxm0r0iD9OKR2LYk4+U\n"
           "1tb25q1KUkGDaNKEBR2QNaA90JF25LHrQ3otuPA1JxMy0eMtKmgB+/QJHKwAxxj/\n"
           "S/0Pujl202JKiay6tBe65GuHWCKtlhnpyXv1nvu6ab+tieYb+bPX/8coSLWJvqm3\n"
           "2vC7KuHh+ODL7mQcGpq3b5TPAuQPpD2smI7O0EAeZs8Qtfo7qZCq0u51K+aHqgT8\n"
           "GeDf3pdNeuBP9bJq86EABQ+2o7mZfV6nzlKwKEutaK7AuE/bZ7+jTVkCAwEAATAN\n"
           "BgkqhkiG9w0BAQsFAAOCAgEAoHc27STV168c5QS1N8h44Qp74LFc+qK080vUKdDC\n"
           "3Ft8ODSi2lMBp5IGZUDKL2euRmwDPWb+SMeAsOrz+neBKPMX024Vv/qF7VCnVV6B\n"
           "6UAT1vRrMWEN1XeOMOzCkqAWlLOQx3miSNzE/rvbRxmBcMdas6gjCwo1psoczHqI\n"
           "8tVrdYAQyDsImcTqNzW3/FrsxjopjYstAYdvRwTuXE+LJV0eJbdN1NrQtZJiZplF\n"
           "1ZiD81SAAJjeb/Ra0/gP1taMCPx3dVb8ooK61lUABQTjZaVLGbNBuoMPxVMC/NIv\n"
           "ZZAZxUQwe4yQKZ2cWCgU5M/vgd/aV7lS+Mi+knab5cfApndgsSEUTm541yA+dgLK\n"
           "1WCJRwoclFy2sVc56cgo7YOLuMtygtPnKgsCv6WAr3kHJIG1g4VXCPx5AFKbc7TR\n"
           "FXl1k4qT35yljpf92rcIfSanPABVNLNMHrC4pyaPgMsZVUgSmBR30ULI+d6KYaPW\n"
           "pH8S7VZ//Yzdnb3/z2sqtXbmiqKNHpR69lplh8M6R/zmJ/N5eGt15d0Y20I1iOGr\n"
           "4qhyC7bC7J35VRIG9BL25v7pObbpyiZQPQOqIUG7HcTDKGXHfgdL/SC1M6XBvvW3\n"
           "73rSiwqlXJwHgxUhR0EO7Yi3kOjrS7d7gjUluZwbar3OaizkSNgFoWr/oe6wG8X7\n"
           "yGw=\n"
           "-----END CERTIFICATE-----";

    std::ofstream myClientKey("to_remove/client.key");
    myClientKey
        << "-----BEGIN RSA PRIVATE KEY-----\n"
           "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\n"
           "-----END RSA PRIVATE KEY-----";

    setenv("CA_CRT", "to_remove/ca.crt", 1);
    setenv("SERVER_CRT", "to_remove/server.crt", 1);
    setenv("SERVER_KEY", "to_remove/server.key", 1);
    setenv("CLIENT_CRT", "to_remove/client.crt", 1);
    setenv("CLIENT_KEY", "to_remove/client.key", 1);
  }
  void TearDown() {
    boost::filesystem::remove_all("to_remove");
  }

  void              runLambda(const bool aSecure);
  const std::string theEndpoint;
};

TEST_F(TestEdgeServerGrpc, test_invalid_endpoint) {
  {
    TrivialEdgeServer myEdgeServer("localhost:100", "my-lambda", "my-bomb");
    EdgeServerGrpc    myEdgeServerGrpc(myEdgeServer, 1, false);
    ASSERT_THROW(myEdgeServerGrpc.run(), std::runtime_error);
  }
  {
    TrivialEdgeServer myEdgeServer("1.2.3.4:10000", "my-lambda", "my-bomb");
    EdgeServerGrpc    myEdgeServerGrpc(myEdgeServer, 1, false);
    ASSERT_THROW(myEdgeServerGrpc.run(), std::runtime_error);
  }
}

void TestEdgeServerGrpc::runLambda(const bool aSecure) {
  // create the server
  TrivialEdgeServer myEdgeServer(theEndpoint, "my-lambda", "my-bomb");
  EdgeServerGrpc    myEdgeServerGrpc(myEdgeServer, 1, aSecure);
  myEdgeServerGrpc.run();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // create the client and run a lambda
  EdgeClientGrpc myClient(theEndpoint, aSecure);
  LambdaRequest  myReq("my-lambda", "Hello world!");
  const auto     myRes(myClient.RunLambda(myReq, false));
  ASSERT_EQ("OK", myRes.theRetCode);
  ASSERT_EQ("HELLO WORLD!", myRes.theOutput);

  // run an unsupported lambda
  LambdaRequest myInvalidReq("not-my-lambda", "Hello world!");
  const auto    myInvalidRes(myClient.RunLambda(myInvalidReq, false));
  ASSERT_EQ("invalid lambda: not-my-lambda", myInvalidRes.theRetCode);
  ASSERT_EQ("", myInvalidRes.theOutput);

  // run a lambda that causes the server to raise an exception
  LambdaRequest myBombReq("my-bomb", "Hello world!");
  const auto    myBombRes(myClient.RunLambda(myBombReq, false));
  ASSERT_EQ("invalid 'my-bomb' request: exception raised",
            myBombRes.theRetCode);
  ASSERT_EQ("", myBombRes.theOutput);
}

TEST_F(TestEdgeServerGrpc, test_run_lambda_insecure) {
  runLambda(false);
}

TEST_F(TestEdgeServerGrpc, test_run_lambda_secure) {
  runLambda(true);
}

} // namespace edge
} // namespace uiiit
