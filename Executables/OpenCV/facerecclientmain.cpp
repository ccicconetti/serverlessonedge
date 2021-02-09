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
#include "Edge/etsiedgeclient.h"
#include "EtsiMec/appcontextmanager.h"
#include "EtsiMec/etsimecoptions.h"
#include "OpenCV/cvutils.h"
#include "Support/checkfiles.h"
#include "Support/chrono.h"
#include "Support/fileutils.h"
#include "Support/glograii.h"
#include "Support/random.h"
#include "Support/saver.h"
#include "Support/signalhandlerflag.h"
#include "Support/split.h"

#include "detectutils.h"

#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/objdetect.hpp"

#include <nlohmann/json.hpp>

#include <glog/logging.h>

#include <boost/program_options.hpp>

#include <cstdlib>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <thread>

using json = nlohmann::json;

namespace po = boost::program_options;
namespace ec = uiiit::edge;

int main(int argc, char* argv[]) {
  const std::string myWndName{"Original image with face detection"};
  const int         myMaxShowWidth{1024};
  const int         myMaxShowHeight{768};

  uiiit::support::GlogRaii myGlogRaii(argv[0]);

  uiiit::support::SignalHandlerFlag mySignalHandler;

  std::string myServerEndpoint;
  std::string myEtsiNotificationUri;
  std::string myLambdaFaces;
  std::string myLambdaEyes;
  std::string myOutputFile;
  double      myOutputTimeOffset;
  std::string myImage;
  std::string mySave;
  double      myLoopDelay;
  double      myDuration;
  std::string myLoopType;
  size_t      myMaxRequests;
  size_t      mySeed;

  po::options_description myDesc("Allowed options");
  // clang-format off
  myDesc.add_options()
    ("server-endpoint",
     po::value<std::string>(&myServerEndpoint)->default_value("localhost:6473"),
     "Server end-point. Overridden by --etsi-ue-app-lcm-proxy-root.")
    ("etsi-notification-uri",
     po::value<std::string>(&myEtsiNotificationUri)->default_value("http://localhost:6600"),
     "Base URI to receive notifications from the ETSI UE application LCM proxy.")
    ("lambda-faces",
     po::value<std::string>(&myLambdaFaces)->default_value("facerec0"),
     "Lambda function name for face detection.")
    ("lambda-eyes",
     po::value<std::string>(&myLambdaEyes)->default_value("eyerec0"),
     "Lambda function name for eyes detection.")
    ("output-file",
     po::value<std::string>(&myOutputFile)->default_value(""),
     "Output file name. If specified suppresses normal output.")
    ("output-append",
     "If specified open the output file in append mode.")
    ("output-time-offset",
     po::value<double>(&myOutputTimeOffset)->default_value(0.0),
     "Use a time offset in the output file")
    ("image",
     po::value<std::string>(&myImage),
     "Path of the file containing the image. "
     "More than one file can be specified (using commas as separators), in which case they are drawn randomly.")
    ("save",
     po::value<std::string>(&mySave),
     "If not empty, save the faces to files with the specified basename.")
    ("loop-delay",
     po::value<double>(&myLoopDelay)->default_value(0.0),
     "Average delay between requests, in s. "
     "Note that if it is not possible to honor the value specified then the actual request rate will be higher than expected.")
    ("duration",
     po::value<double>(&myDuration)->default_value(0.0),
     "Terminate loop after the given duration, in s; 0 means never.")
    ("loop-type",
     po::value<std::string>(&myLoopType)->default_value("constant"),
     "One of: constant, uniform, poisson.")
    ("max-requests",
     po::value<size_t>(&myMaxRequests)->default_value(std::numeric_limits<size_t>::max()),
     "Number of requests to issue, provided that --loop is used.")
    ("eyes", "Detect eyes in each image.")
    ("show", "Display image with face detection rectangles (incompatible with camera/loop).")
    ("camera", "Stream continuously from camera and show to video (image/save options unused, incompatible with show/loop).")
    ("loop", "Continuously request detection on the same image (incompatible with show/camera).")
    ("seed",
     po::value<size_t>(&mySeed)->default_value(std::chrono::system_clock::now().time_since_epoch().count()),
     "Seed generator. If not specified use the number of seconds since Jan 1, 1970.")
    ;
  // clang-format on

  try {
    uiiit::etsimec::EtsiMecOptions myCli(argc, argv, "", false, myDesc);

    if (myServerEndpoint.empty() and myCli.apiRoot().empty()) {
      throw std::runtime_error(
          "Both --server and --etsi-ue-app-lcm-proxy-root are empty");
    }
    LOG_IF(WARNING,
           not myServerEndpoint.empty() and not myCli.apiRoot().empty())
        << "Both --server and --etsi-ue-app-lcm-proxy-root are specified: "
           "ignore the former";

    std::unique_ptr<ec::EdgeClientInterface>           myEdgeClient;
    std::unique_ptr<uiiit::etsimec::AppContextManager> myContextManager;
    if (not myCli.apiRoot().empty()) {
      if (myEtsiNotificationUri.empty()) {
        throw std::runtime_error(
            "The ETSI notification URI cannot be unspecified if the client is "
            "expected to connect to and ETSI UE application LCM proxy");
      }
      myContextManager.reset(new uiiit::etsimec::AppContextManager(
          myEtsiNotificationUri, myCli.apiRoot()));
      myContextManager->start();
      myEdgeClient.reset(new ec::EtsiEdgeClient(*myContextManager));

    } else {
      // establish direct connection with the edge computer/router
      myEdgeClient.reset(new ec::EdgeClientGrpc(myServerEndpoint));
    }

    if (((myCli.varMap().count("camera") > 0) +
         (myCli.varMap().count("loop") > 0) +
         (myCli.varMap().count("show") > 0)) > 1) {
      throw std::runtime_error("Pick only one of: camera, loop, show");
    }

    if (std::set<std::string>({"poisson", "constant", "uniform"})
            .count(myLoopType) == 0) {
      throw std::runtime_error(
          "Invalid loop type, choose one of: constant, uniform, poisson");
    }

    // parse the set of images to be used, if any
    const auto myImages =
        uiiit::support::split<std::vector<std::string>>(myImage, ",");

    // will contain faces and eyes rectangles detected
    std::vector<cv::Rect> myFaces;
    std::vector<cv::Rect> myEyes;

    if (myCli.varMap().count("camera") > 0) {
      //
      // camera mode
      //
      cv::VideoCapture myCapture;
      myCapture.open(0);
      if (not myCapture.isOpened()) {
        throw std::runtime_error("Could not open camera");
      }

      cv::Mat myImg;
      while (myCapture.read(myImg) and not mySignalHandler.flag()) {
        const auto ret = cv::detect(*myEdgeClient,
                                    myLambdaFaces,
                                    myLambdaEyes,
                                    myImg,
                                    myFaces,
                                    myCli.varMap().count("eyes") > 0,
                                    myEyes,
                                    true);

        std::cout << "elapsed " << (ret.first * 1e3 + 0.5) << " ms, load "
                  << ret.second << ", detected " << myFaces.size()
                  << " faces and " << myEyes.size() << " eyes" << std::endl;

        cv::addFacesEyes(myImg, myFaces, myEyes);
        cv::addElapsed(myImg, ret.first);

        cv::imshow(myWndName, myImg);

        if (cv::waitKey(10) == 27) {
          break;
        }
      }

    } else {
      //
      // image from file mode
      //

      if (myImages.empty()) {
        throw std::runtime_error("No image file specified");
      }

      // read and decode original images from files
      std::vector<cv::Mat> myImgs;
      for (const auto& myImageFile : myImages) {
        uiiit::support::checkFiles(std::set<std::string>({myImageFile}));
        myImgs.emplace_back(cv::imread(myImageFile, cv::IMREAD_COLOR));
        if (myImgs.back().empty()) {
          throw std::runtime_error("Could not load image: " + myImageFile);
        }
      }
      uiiit::support::UniformIntRv<unsigned int> myUnifRv(
          0, myImgs.size() - 1, mySeed, 0, 0);

      // used only with looped requests
      std::unique_ptr<uiiit::support::RealRvInterface> myRv;
      if (myCli.varMap().count("loop") > 0 and myLoopDelay > 0) {
        if (myLoopType == "poisson") {
          myRv.reset(new uiiit::support::ExponentialRv(
              1.0 / myLoopDelay, mySeed, 0, 0));
        } else if (myLoopType == "uniform") {
          myRv.reset(new uiiit::support::UniformRv(
              0.0, 2.0 * myLoopDelay, mySeed, 0, 0));
        } else if (myLoopType == "constant") {
          myRv.reset(new uiiit::support::ConstantRv(myLoopDelay));
        }
      }

      // open the output file, with timestamp and flushing at every
      // measurement
      const uiiit::support::Saver mySaver(
          myOutputFile,
          true,
          true,
          myCli.varMap().count("output-append") > 0,
          myOutputTimeOffset);

      const auto myCounterLoop =
          myCli.varMap().count("loop") > 0 ? myMaxRequests : size_t(1);
      uiiit::support::Chrono mySleepChrono(false);
      uiiit::support::Chrono myDurationChrono(true);
      for (size_t i = 0; i < myCounterLoop; i++) {
        // break if the program was interrupted by user
        if (mySignalHandler.flag()) {
          break;
        }

        // break if we exceed the overall time
        if (myDuration > 0 and myDurationChrono.time() > myDuration) {
          break;
        }

        // select a random image from the bouquet
        auto& myImg = myImgs[myUnifRv()];

        // wait a random (Poisson distributed or constant) time, unless this
        // is the very first request or the user specified back-to-back tx
        if (i > 0 and myLoopDelay > 0) {
          assert(myLoopType == "poisson" or myLoopType == "constant");
          assert(myCli.varMap().count("loop") > 0);

          const auto myExpectedSleep = myRv ? (*myRv)() : myLoopDelay; // in s
          const auto myActualSleep =
              myExpectedSleep - mySleepChrono.stop(); // in s
          if (myActualSleep < 0) {
            LOG_FIRST_N(WARNING, 1) << "the actual request rate may differ "
                                       "from the expected rate";
          } else {
            std::this_thread::sleep_for(std::chrono::nanoseconds(
                static_cast<long long>(0.5 + 1e9 * myActualSleep)));
          }
        }

        // with looped requests, this is used to compute the sleep time
        mySleepChrono.start();

        // perform face (and eyes, if enabled) detection
        const auto ret = cv::detect(*myEdgeClient,
                                    myLambdaFaces,
                                    myLambdaEyes,
                                    myImg,
                                    myFaces,
                                    myCli.varMap().count("eyes") > 0,
                                    myEyes,
                                    false);

        if (myOutputFile.empty()) {
          std::cout << "elapsed " << (ret.first * 1e3 + 0.5) << " ms, load "
                    << ret.second << ", detected " << myFaces.size()
                    << " faces and " << myEyes.size() << " eyes" << std::endl;
        } else {
          mySaver(ret.first, ret.second);
        }

        // save images, one per face/eye (original size, no extra rendering)
        // if looping then we only save the first result
        if (not mySave.empty() and i == 0) {
          size_t myCnt = 0;
          for (const auto& myFace : myFaces) {
            const auto myOutFilename =
                mySave + "-face-" + std::to_string(myCnt++) + ".png";
            if (not cv::imwrite(myOutFilename,
                                myImg(myFace),
                                {cv::IMWRITE_PNG_COMPRESSION, 9})) {
              throw std::runtime_error("Error while saving to " +
                                       myOutFilename);
            }
          }
          myCnt = 0;
          for (const auto& myEye : myEyes) {
            const auto myOutFilename =
                mySave + "-eye-" + std::to_string(myCnt++) + ".png";
            if (not cv::imwrite(myOutFilename,
                                myImg(myEye),
                                {cv::IMWRITE_PNG_COMPRESSION, 9})) {
              throw std::runtime_error("Error while saving to " +
                                       myOutFilename);
            }
          }
        }

        // show the original image (possibly scaled down) with rectangles
        // around faces and respective eyes
        if (myCli.varMap().count("show") > 0) {
          assert(myCounterLoop == 1);

          cv::addFacesEyes(myImg, myFaces, myEyes);
          cv::addElapsed(myImg, ret.first);

          // resize the image if too big to show
          cv::Mat*                 mySelectedImg = &myImg;
          std::unique_ptr<cv::Mat> myScaledImg;
          if (myImg.cols > myMaxShowWidth or myImg.rows > myMaxShowHeight) {
            myScaledImg.reset(new cv::Mat());
            const auto myScale = std::min(double(myMaxShowWidth) / myImg.cols,
                                          double(myMaxShowHeight) / myImg.rows);
            cv::resize(myImg,
                       *myScaledImg,
                       cv::Size(),
                       myScale,
                       myScale,
                       cv::INTER_AREA);
            mySelectedImg = myScaledImg.get();
          }

          // show a window with the image
          cv::namedWindow(myWndName, cv::WINDOW_AUTOSIZE);
          cv::imshow(myWndName, *mySelectedImg);
          cv::waitKey(0); // until key pressed
        }
      }
    }

    return EXIT_SUCCESS;
  } catch (const uiiit::support::CliExit&) {
    return EXIT_SUCCESS; // clean exit
  } catch (const std::exception& aErr) {
    LOG(ERROR) << "Exception caught: " << aErr.what();
  } catch (...) {
    LOG(ERROR) << "Unknown exception caught";
  }

  return EXIT_FAILURE;
}
