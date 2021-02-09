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

#include "printtable.h"

#include "Support/tostring.h"

#include <cmath>

namespace uiiit {
namespace edge {
namespace detail {

void printTable(
    std::ostream& aStream,
    const std::map<std::string, std::map<std::string, std::pair<float, bool>>>&
        aTable) {
  size_t myMaxLen       = 0;
  size_t myMaxLenWeight = 0;
  for (const auto& myEntry : aTable) {
    myMaxLen = std::max(myMaxLen, myEntry.first.size());
    for (const auto& myDestination : myEntry.second) {
      myMaxLenWeight =
          std::max(myMaxLenWeight, toString(myDestination.second.first).size());
    }
  }
  for (const auto& myEntry : aTable) {
    auto myFirst = true;
    for (const auto& myDestination : myEntry.second) {
      if (myFirst) {
        myFirst = false;
        aStream << myEntry.first
                << std::string(myMaxLen - myEntry.first.size(), ' ');
      } else {
        aStream << std::string(myMaxLen, ' ');
      }
      const auto myWeight = toString(myDestination.second.first);
      aStream << " [" << myWeight
              << std::string(myMaxLenWeight - myWeight.size(), ' ') << "] "
              << myDestination.first
              << (myDestination.second.second ? " (F)" : "") << '\n';
    }
  }
}

} // namespace detail
} // namespace edge
} // namespace uiiit
