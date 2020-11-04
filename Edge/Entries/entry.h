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

#include <list>
#include <map>
#include <string>

namespace uiiit {
namespace edge {
namespace entries {

/**
 * Generic entry of an edge router forwarding table.
 */
class Entry
{
 protected:
  struct Element final {
    std::string theDestination;
    float       theWeight;
    bool        theFinal;
    bool        operator<(const Element& aOther) const noexcept {
      return theWeight < aOther.theWeight;
    }
  };

 public:
  //! An entry with no destinations.
  explicit Entry();

  virtual ~Entry() {
  }

  //! \return True if there are no destinations.
  bool empty() const noexcept {
    return theDestinations.empty();
  }

  /**
   * Return a random destination according to the weights.
   *
   * \throw NoDestinations if the current entry is empty.
   */
  virtual std::string operator()() = 0;

  /**
   * Add a new destination or change the weight of a destination.
   *
   * \param aDest The destination to be added or whose weight has to be
   * changed.
   *
   * \param aWeight The destination weight.
   *
   * \param aFinal True if the destination is final.
   *
   * \throw InvalidDestination if the destination is empty of the weight is
   * non-positive.
   */
  void change(const std::string& aDest, const float aWeight, const bool aFinal);

  /**
   * Change the weight of an existing destination.
   *
   * \param aDest The destination to be added or whose weight has to be
   * changed.
   *
   * \param aWeight The destination weight.
   *
   * \throw NoDestinations if aDest is empty or the destination does not
   * exist.
   *
   * \throw InvalidDestination if the weight is non-positive.
   */
  void change(const std::string& aDest, const float aWeight);

  /**
   * Remove a destination
   *
   * \return True if the destination has been actually removed.
   */
  bool remove(const std::string& aDest);

  /**
   * \return the weight of a given destination.
   *
   * \throw NoDestinations if aDest is empty or the destination does not
   * exist.
   */
  float weight(const std::string& aDest) const;

  //! \return All the destinations, weights, and final flags.
  std::map<std::string, std::pair<float, bool>> destinations() const;

  //! print content of thePFstats (used for testing)
  void printPFstats();

 private:
  virtual void updateWeight(const std::string& aDest,
                            const float        aOldWeight,
                            const float        aNewWeight)                         = 0;
  virtual void updateAddDest(const std::string& aDest, const float aWeight) = 0;
  virtual void updateDelDest(const std::string& aDest, const float aWeight) = 0;

 protected:
  std::list<Element> theDestinations;

  /**
   * data structure used only if the scheduling policy is PF:
   * - the string is the destination (field theDestination in datastructure
   *   Element);
   * - pair.first is theLambdaServedCount related to the destination,
   *   datastructure
   * - pair.second is theTimestamp in which the destination joined the system
   *   or has served the last Lambda Request
   */
  std::map<std::string, std::pair<int, double>> thePFstats;
};

} // namespace entries
} // namespace edge
} // namespace uiiit
