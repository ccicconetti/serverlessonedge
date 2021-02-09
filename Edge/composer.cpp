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

#include "composer.h"

#include "Support/conf.h"
#include "Support/tostring.h"
#include "computer.h"
#include "lambda.h"
#include "processortype.h"

#include <glog/logging.h>

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace uiiit {
namespace edge {

void Composer::operator()(const support::Conf& aConf,
                          Computer&            aComputer) const {
  static const auto GB     = uint64_t(1) << uint64_t(30);
  const auto        myType = aConf("type");
  if (myType == "raspberry") {
    // add processors
    aComputer.addProcessor("arm", ProcessorType::GenericCpu, 1e11, 4, GB / 2);
    aComputer.addProcessor(
        "bm2837", ProcessorType::GenericGpu, 1e11, 1, GB / 2);

    // add containers, depending on the configuration
    auto myNumCpuContainers = aConf.getUint("num-cpu-containers");
    while (myNumCpuContainers-- > 0) {
      const auto myNumCpuWorkers = aConf.getUint("num-cpu-workers");
      const auto myId            = std::to_string(myNumCpuContainers);
      aComputer.addContainer(
          "ccontainer" + myId,
          "arm",
          Lambda("clambda" + myId,
                 ProportionalRequirements(1e6, 4 * 1e6, 100, 0)),
          myNumCpuWorkers);
    }

    auto myNumGpuContainers = aConf.getUint("num-gpu-containers");
    while (myNumGpuContainers-- > 0) {
      const auto myNumGpuWorkers = aConf.getUint("num-gpu-workers");
      const auto myId            = std::to_string(myNumGpuContainers);
      aComputer.addContainer(
          "gcontainer" + myId,
          "bm2837",
          Lambda("glambda" + myId, ProportionalRequirements(1e6, 1e6, 100, 0)),
          myNumGpuWorkers);
    }

  } else if (myType == "intel-server") {
    // add processors
    aComputer.addProcessor(
        "xeon", ProcessorType::GenericCpu, 4e9, 20, 128 * GB);

    // add containers, depending on the configuration
    auto       myNumCpuContainers = aConf.getUint("num-containers");
    const auto myNumCpuWorkers    = aConf.getUint("num-workers");
    while (myNumCpuContainers-- > 0) {
      const auto myId = std::to_string(myNumCpuContainers);
      aComputer.addContainer(
          "ccontainer" + myId,
          "xeon",
          Lambda("clambda" + myId,
                 ProportionalRequirements(1e6, 4 * 1e6, 100, 0)),
          myNumCpuWorkers);
    }

  } else if (myType == "file") {
    struct CheckExistence {
      void operator()(const json& aJson, const std::string& aKey) const {
        if (aJson.count(aKey) == 0) {
          throw std::runtime_error("Missing key: " + aKey);
        }
      }
      void operator()(const json&                  aJson,
                      const std::set<std::string>& aKeys) const {
        for (const auto& myKey : aKeys) {
          (*this)(aJson, myKey);
        }
      }
    };

    const auto    myPath = aConf("path");
    std::ifstream myFile(myPath);
    if (not myFile) {
      throw std::runtime_error("Could not open configuration file: " + myPath);
    }

    // deserialize JSON from file
    json myJson;
    myFile >> myJson;

    // check version
    CheckExistence()(myJson, "version");
    const auto& myVersion = myJson["version"].get<std::string>();
    if (myVersion != "1.0") {
      throw std::runtime_error("Invalid version found: " + myVersion);
    }

    // add processors
    CheckExistence()(myJson, "processors");
    for (const auto& myProcessor : myJson["processors"]) {
      CheckExistence()(myProcessor,
                       {"name", "type", "speed", "cores", "memory"});
      aComputer.addProcessor(myProcessor["name"],
                             processorTypeFromString(myProcessor["type"]),
                             myProcessor["speed"],
                             myProcessor["cores"],
                             myProcessor["memory"]);
    }

    // add containers
    CheckExistence()(myJson, "containers");
    for (const auto& myContainer : myJson["containers"]) {
      CheckExistence()(myContainer,
                       {"lambda", "name", "processor", "num-workers"});
      const auto& myLambda = myContainer["lambda"];
      CheckExistence()(myLambda,
                       {"requirements",
                        "name",
                        "output-type",
                        "op-coeff",
                        "op-offset",
                        "mem-coeff",
                        "mem-offset"});
      const auto& myRequirements = myLambda["requirements"].get<std::string>();
      if (myRequirements != "proportional") {
        throw std::runtime_error("Unsupported requirements in lambda: " +
                                 myRequirements);
      }
      const ProportionalRequirements myReqs(myLambda["op-coeff"],
                                            myLambda["op-offset"],
                                            myLambda["mem-coeff"],
                                            myLambda["mem-offset"]);

      std::unique_ptr<const Lambda> myLambdaPtr;
      const auto myOutputType = myLambda["output-type"].get<std::string>();
      if (myOutputType == "copy-input") {
        myLambdaPtr.reset(new Lambda(myLambda["name"], myReqs));
      } else if (myOutputType == "fixed") {
        CheckExistence()(myLambda, "output-value");
        myLambdaPtr.reset(
            new Lambda(myLambda["name"],
                       myLambda["output-value"].get<std::string>(),
                       myReqs));
      } else {
        throw std::runtime_error("Unsupported output-type in lambda: " +
                                 myOutputType);
      }

      aComputer.addContainer(myContainer["name"],
                             myContainer["processor"],
                             *myLambdaPtr,
                             myContainer["num-workers"]);
    }

  } else {
    throw std::runtime_error("Invalid type: " + myType +
                             ", possible values: " + toString(types(), ", "));
  }
  LOG(INFO) << aComputer;
}

std::string Composer::jsonExample() noexcept {
  return "{\n"
         "\t\"version\": \"1.0\",\n"
         "\t\"processors\": [{\n"
         "\t\t\"name\": \"arm\",\n"
         "\t\t\"type\": \"GenericCpu\",\n"
         "\t\t\"speed\": 4e9,\n"
         "\t\t\"memory\": 500e6,\n"
         "\t\t\"cores\": 1\n"
         "\t}],\n"
         "\n"
         "\t\"containers\": [{\n"
         "\t\t\"name\": \"ccontainer0\",\n"
         "\t\t\"processor\": \"arm\",\n"
         "\t\t\"lambda\": {\n"
         "\t\t\t\"name\": \"clambda@1\",\n"
         "\t\t\t\"output-type\": \"copy-input\",\n"
         "\t\t\t\"requirements\": \"proportional\",\n"
         "\t\t\t\"op-coeff\": 1e6,\n"
         "\t\t\t\"op-offset\": 4e6,\n"
         "\t\t\t\"mem-coeff\": 100,\n"
         "\t\t\t\"mem-offset\": 0\n"
         "\t\t},\n"
         "\t\t\"num-workers\": 1\n"
         "\t}]\n"
         "}";
}

const std::set<std::string>& Composer::types() {
  static const std::set<std::string> myTypes(
      {"raspberry", "intel-server", "file"});
  return myTypes;
}

} // namespace edge
} // namespace uiiit
