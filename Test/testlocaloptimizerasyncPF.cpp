#include "Edge/edgerouter.h"
#include "Edge/forwardingtable.h"
#include "Edge/lambda.h"
#include "Edge/localoptimizerasyncPF.h"
#include "Edge/localoptimizerfactory.h"
#include "Support/conf.h"

#include "gtest/gtest.h"

namespace uiiit {
namespace edge {

struct TestLocalOptimizerAsyncPF : public ::testing::Test {};

TEST_F(TestLocalOptimizerAsyncPF, test_ctor) {

  ForwardingTable ft(ForwardingTable::Type::ProportionalFairness);
  std::shared_ptr<LocalOptimizer> optimizer(LocalOptimizerFactory::make(
      ft, support::Conf("type=asyncPF,alpha=1,beta=2")));
  ASSERT_NE(optimizer, nullptr);
}

TEST_F(TestLocalOptimizerAsyncPF, test_functional_operator) {

  ForwardingTable ft(ForwardingTable::Type::ProportionalFairness);

  ft.change("lambda1", "dest1:666", 1, true);
  ft.change("lambda1", "dest2:666", 0.5, false);
  ft.change("lambda1", "dest2:667", 99, true);

  ft.change("lambda2", "dest1:666", 1, true);
  ft.change("lambda2", "dest2:666", 0.5, false);

  ft.change("another_lambda", "dest3:666", 42, false);

  std::shared_ptr<LocalOptimizer> optimizer(LocalOptimizerFactory::make(
      ft, support::Conf("type=asyncPF,alpha=1,beta=2")));
  ASSERT_NE(optimizer, nullptr);

  LambdaRequest myReq("lambda1", "");

  (*optimizer)(myReq.toProtobuf(), std::string("dest1:666"), double(2)); 
}

} // namespace edge
} // namespace uiiit