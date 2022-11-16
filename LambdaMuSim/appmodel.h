/*
              __ __ __
             |__|__|  | __
             |  |  |  ||__|
  ___ ___ __ |  |  |  |
 |   |   |  ||  |  |  |    Ubiquitous Internet @ IIT-CNR
 |   |   |  ||  |  |  |    C++ edge computing libraries and tools
 |_______|__||__|__|__|    https://github.com/ccicconetti/serverlessonedge

Licensed under the MIT License <http://opensource.org/licenses/MIT>
Copyright (c) 2022 C. Cicconetti <https://ccicconetti.github.io/>

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

#include "Support/macros.h"
#include "Support/random.h"

#include <map>
#include <memory>
#include <string>

namespace uiiit {
namespace lambdamusim {

/**
 * @brief Create an AppModel object based on a string description.
 *
 * @param aSeed the seed to initialize the random number generators.
 * @param aDescription the description of the AppModel to make.
 *
 * @throw std::runtime_error if the description is invalid.
 *
 * Examples of description:
 *
 * "constant,12,1000,420"
 * -> create an AppModelConstant with parameters:
 *    - 12 service rate, in functions/time unit
 *    - 1000 exchange size, in data units/invocation
 *    - 420 storage size, in data units/invocation
 *
 * "classes,1,12,1000,420,2,6,500,210"
 * -> create an AppModelClasses with two classes:
 *    1. the first class of applications has the same parameters
 *       as the AppModelConstant example above
 *    2. the second class of applications has all values halved
 *       but has twice the probability of being drawn than the
 *       first class of applications
 */
class AppModel;
std::unique_ptr<AppModel> makeAppModel(const size_t       aSeed,
                                       const std::string& aDescription);

/**
 * @brief An abstract application model.
 *
 * By invoking the function operator on instances of specialized classes, the
 * user will get each time a random new set of application parameters.
 */
class AppModel
{
  NONCOPYABLE_NONMOVABLE(AppModel);

 public:
  struct Params {
    long theServiceRate  = 1; //!< the rate of function invocations
    long theExchangeSize = 1; //!< amount of data units per invocation
    long theStorageSize  = 1; //!< state size in the remote storage

    std::string toString() const;
  };

  AppModel()          = default;
  virtual ~AppModel() = default;

  virtual Params operator()() = 0;
};

/**
 * @brief Return always the same parameters given as input to the ctor.
 */
class AppModelConstant final : public AppModel
{
 public:
  /**
   * @brief Construct a new App Model Classes object
   *
   * @param aParam The parameters returned.
   */
  AppModelConstant(const Params& aParams);
  Params operator()() override;

 private:
  const Params theParams;
};

/**
 * @brief Applications belong to a number of classes, each with well-defined
 * parameters.
 */
class AppModelClasses final : public AppModel
{
 public:
  /**
   * @brief Construct a new App Model Classes object
   *
   * @param aSeed The seed to initialize the random number generator.
   * @param aParams The parameters of the classes, with the key being a
   * factor proportional to the probability that an application from that class
   * is drawn.
   *
   * @throw std::runtime_error if aParams is empty.
   */
  AppModelClasses(const size_t                         aSeed,
                  const std::multimap<double, Params>& aParams);
  Params operator()() override;

 private:
  support::UniformRv                  theRv;
  const std::multimap<double, Params> theParams;
};

} // namespace lambdamusim
} // namespace uiiit
