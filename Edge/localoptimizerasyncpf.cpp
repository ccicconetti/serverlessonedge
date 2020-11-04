#include "localoptimizerasyncpf.h"
#include "forwardingtable.h"
#include "edgeserver.grpc.pb.h"

#include <glog/logging.h>
#include <cassert>

namespace uiiit {
namespace edge {

LocalOptimizerAsyncPF::LocalOptimizerAsyncPF(ForwardingTable& aForwardingTable)
    : LocalOptimizer(aForwardingTable) {
  LOG(INFO) << "Creating an async local optimizer for proportional fairness "
            << '\n';
}

void LocalOptimizerAsyncPF::operator()(const rpc::LambdaRequest& aReq,
                                       const std::string&        aDestination,
                                       const double              aTime) {
  assert(aTime > 0);
  const auto& myLambda = aReq.name();
  theForwardingTable.change(myLambda, aDestination, aTime);
}

} // namespace edge
} // namespace uiiit
