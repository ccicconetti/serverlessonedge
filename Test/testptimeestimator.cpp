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

#include "Edge/edgemessages.h"
#include "Edge/ptimeestimator.h"
#include "Support/tostring.h"

#include "gtest/gtest.h"

#include <list>

namespace uiiit {
namespace edge {

struct TrivialPtimeEstimator final : PtimeEstimator {
  TrivialPtimeEstimator()
      : PtimeEstimator(Type::Test) {
  }

  std::string operator()(const rpc::LambdaRequest& aReq) override {
    std::ignore = aReq;
    return "";
  }

  void processSuccess(const rpc::LambdaRequest& aReq,
                      const std::string&        aDestination,
                      const LambdaResponse&     aRep,
                      const double              aTime) override {
    std::ignore = aReq;
    std::ignore = aDestination;
    std::ignore = aRep;
    std::ignore = aTime;
  }

  struct Command {
    enum CommandType { ADDED = 0, REMOVED = 1 };

    CommandType theCommand;
    std::string theLambda;
    std::string theDestination;

    bool operator==(const Command& aOther) const {
      return theCommand == aOther.theCommand and
             theLambda == aOther.theLambda and
             theDestination == aOther.theDestination;
    }

    std::string toString() const {
      return std::string() + (theCommand == ADDED ? "add " : "del ") +
             theLambda + " " + theDestination;
    }
  };

  void privateAdd(const std::string& aLambda,
                  const std::string& aDestination) override {
    theCommands.emplace_back(Command{Command::ADDED, aLambda, aDestination});
  }
  void privateRemove(const std::string& aLambda,
                     const std::string& aDestination) override {
    theCommands.emplace_back(Command{Command::REMOVED, aLambda, aDestination});
  }

  std::list<Command> theCommands;
};

struct TestPtimeEstimator : public ::testing::Test {};

TEST_F(TestPtimeEstimator, test_ctor) {
  ASSERT_NO_THROW(TrivialPtimeEstimator());
}

TEST_F(TestPtimeEstimator, test_add_remove) {
  using OutMap =
      std::map<std::string, std::map<std::string, std::pair<float, bool>>>;
  using Set = std::set<std::string>;

  OutMap                myExp;
  TrivialPtimeEstimator myEst;

  // add new lambda
  myExp["lambda1"]["dest1"] = {1.0f, true};
  myEst.change("lambda1", "dest1", -42.0f, true);
  ASSERT_EQ(myExp, myEst.fullTable());
  ASSERT_EQ(Set{"lambda1"}, myEst.lambdas());

  // add new lambda
  myExp["lambda2"]["dest2"] = {1.0f, false};
  myEst.change("lambda2", "dest2", -42.0f, false);
  ASSERT_EQ(myExp, myEst.fullTable());
  ASSERT_EQ(Set({"lambda1", "lambda2"}), myEst.lambdas());

  // add new destination
  myExp["lambda2"]["dest3"] = {1.0f, true};
  myEst.change("lambda2", "dest3", -42.0f, true);
  ASSERT_EQ(myExp, myEst.fullTable());
  ASSERT_EQ(Set({"lambda1", "lambda2"}), myEst.lambdas());

  // do nothing
  myEst.change("lambda1", "dest1", 100.0f);
  myEst.change("lambda2", "dest2", 100.0f);
  myEst.change("lambda2", "dest3", 100.0f);
  ASSERT_EQ(myExp, myEst.fullTable());
  ASSERT_EQ(Set({"lambda1", "lambda2"}), myEst.lambdas());

  // remove lambda2
  myEst.remove("lambda2");
  myExp.erase("lambda2");
  ASSERT_EQ(myExp, myEst.fullTable());
  ASSERT_EQ(Set({"lambda1"}), myEst.lambdas());

  // add new destination
  myExp["lambda1"]["dest9"] = {1.0f, true};
  myEst.change("lambda1", "dest9", 100.0f, true);
  ASSERT_EQ(myExp, myEst.fullTable());
  ASSERT_EQ(Set({"lambda1"}), myEst.lambdas());

  // remove it
  myExp["lambda1"].erase("dest9");
  myEst.remove("lambda1", "dest9");
  ASSERT_EQ(myExp, myEst.fullTable());
  ASSERT_EQ(Set({"lambda1"}), myEst.lambdas());

  // remove non-existing destination
  myEst.remove("lambda1", "destX");
  ASSERT_EQ(myExp, myEst.fullTable());
  ASSERT_EQ(Set({"lambda1"}), myEst.lambdas());

  // remove non-existing lambda
  myEst.remove("lambdaX");
  ASSERT_EQ(myExp, myEst.fullTable());
  ASSERT_EQ(Set({"lambda1"}), myEst.lambdas());

  // remove the only remaining entry
  myEst.remove("lambda1", "dest1");
  ASSERT_EQ(OutMap(), myEst.fullTable());
  ASSERT_EQ(Set(), myEst.lambdas());

  using Command = TrivialPtimeEstimator::Command;
  ASSERT_EQ(std::list<Command>({
                Command{Command::ADDED, "lambda1", "dest1"},
                Command{Command::ADDED, "lambda2", "dest2"},
                Command{Command::ADDED, "lambda2", "dest3"},
                Command{Command::REMOVED, "lambda2", "dest2"},
                Command{Command::REMOVED, "lambda2", "dest3"},
                Command{Command::ADDED, "lambda1", "dest9"},
                Command{Command::REMOVED, "lambda1", "dest9"},
                Command{Command::REMOVED, "lambda1", "dest1"},
            }),
            myEst.theCommands);
}

TEST_F(TestPtimeEstimator, test_print) {
  TrivialPtimeEstimator myEst;
  ASSERT_EQ(std::string(), ::toString(myEst));

  myEst.change("lambda1", "dest1", -42.0f, true);
  myEst.change("lambda1", "dest2", -42.0f, false);
  myEst.change("lambda2", "dest1", -42.0f, true);
  ASSERT_EQ(
      std::string("lambda1 [1] dest1 (F)\n        [1] dest2\nlambda2 [1] dest1 (F)\n"),
      ::toString(myEst));
}

} // namespace edge
} // namespace uiiit
