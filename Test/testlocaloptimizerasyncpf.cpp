#include "Edge/edgerouter.h"
#include "Edge/forwardingtable.h"
#include "Edge/forwardingtableexceptions.h"
#include "Edge/forwardingtableinterface.h"
#include "Edge/lambda.h"
#include "Edge/localoptimizerasyncpf.h"
#include "Edge/localoptimizerfactory.h"
#include "Support/conf.h"
#include "Support/tostring.h"

#include "gtest/gtest.h"

#include <glog/logging.h>

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
  
  //fakeFill(ft,2,3);
  // LOG(INFO) << "Forwarding Table dopo fakefill = "
  //           << ::toString(ft) << '\n';

  //ASSERT_THROW(ft("lambda1"), NoDestinations);

  // INSERITO ALTRIMENTI MI Dà ECCEZIONE NoDestination 3° IF ForwardingTable
  ft.change("lambda1", "dest1:666", 1.0f, true);
  LOG(INFO) << "Forwarding Table dopo PRIMO change = "
            << ::toString(ft);

  std::shared_ptr<LocalOptimizer> optimizer(LocalOptimizerFactory::make(
      ft, support::Conf("type=asyncPF,alpha=1,beta=2")));
  ASSERT_NE(optimizer, nullptr);

  LambdaRequest myReq("lambda1", "batman");
  (*optimizer)(myReq.toProtobuf(), std::string("dest1:666"), double(2));
  LOG(INFO) << "Forwarding Table dopo aver inserito dest1:666 con latenza 2 = "
            << ::toString(ft);

  // (*optimizer)(myReq.toProtobuf(), std::string("dest1:666"), double(2));
  // LOG(INFO) << "Forwarding Table dopo aver inserito SECONDO dest1:666 con
  // latenza 2 = " << ::toString(ft);

  // (*optimizer)(myReq.toProtobuf(), std::string("dest1:666"), double(3));
  // LOG(INFO) << "Forwarding Table dopo aver inserito TERZO dest1:666 con
  // latenza 3 = " << ::toString(ft);

  // LOG(INFO) << "FULLTABLE()" <<
  // std::map<std::string, std::map<std::string, std::pair<float, bool>>>

  // LOG(INFO) << "PESO = " <<
  // ft.destinations("lambda1").find("dest1:666")->second.first;
}

} // namespace edge
} // namespace uiiit