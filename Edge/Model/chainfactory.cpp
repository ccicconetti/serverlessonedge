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

#include "Edge/Model/chainfactory.h"

#include "Edge/Model/chain.h"
#include "Support/conf.h"
#include "Support/fileutils.h"

#include <nlohmann/json.hpp>

#include <cassert>
#include <fstream>

using json = nlohmann::json;

namespace uiiit {
namespace edge {
namespace model {

void ChainFactory::make(const support::Conf&           aConf,
                        std::unique_ptr<Chain>&        aChain,
                        std::map<std::string, size_t>& aStateSizes) {
  assert(aChain.get() == nullptr);
  assert(aStateSizes.empty());

  const auto myType = aConf("type");
  if (myType.empty()) {
    throw std::runtime_error("No 'type' field found in chain configuration");
  }

  if (myType == "make-template") {
    std::ofstream myOutfile(exampleFileName());
    myOutfile << exampleChain();

    aChain = nullptr;
  } else if (myType == "file") {
    const auto myFilename = aConf("filename");
    if (myFilename.empty()) {
      throw std::runtime_error(
          "No 'filename' field found in chain configuration");
    }

    // read entire file
    const auto myFileAsString = support::readFileAsString(myFilename);

    // create chain
    aChain.reset(new Chain(Chain::fromJson(myFileAsString)));

    // read state sizes
    const auto myJson = json::parse(myFileAsString);
    for (const auto& elem : myJson["state-sizes"].items()) {
      aStateSizes.emplace(elem.key(), elem.value());
    }

  } else {
    throw std::runtime_error(
        "Invalid 'type' field found in chain configuration");
  }
}

std::string ChainFactory::exampleFileName() {
  return "chain-example.json";
}

std::string ChainFactory::exampleChain() {
  return "{\n"
         "  \"functions\" : [ \"f1\", \"f2\", \"f1\" ],\n"
         "  \"dependencies\" :\n"
         "    {\n"
         "    \"s0\" : [ \"f1\" ],\n"
         "    \"s1\" : [ \"f1\", \"f2\" ],\n"
         "    \"s2\" : [ \"f2\" ],\n"
         "    \"s3\" : null\n"
         "    },\n"
         "  \"state-sizes\" :\n"
         "    {\n"
         "    \"s0\" : 10000,\n"
         "    \"s1\" : 20000,\n"
         "    \"s2\" : 30000,\n"
         "    \"s3\" : 40000\n"
         "    }\n"
         "}\n";
}

} // namespace model
} // end namespace edge
} // end namespace uiiit
