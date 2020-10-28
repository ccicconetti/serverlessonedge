#include "localoptimizerasyncPF.h"

#include "forwardingtable.h"

#include "edgeserver.grpc.pb.h"

#include <glog/logging.h>

#include <cassert>

#include <cmath>

namespace uiiit {
namespace edge {

LocalOptimizerAsyncPF::LocalOptimizerAsyncPF(ForwardingTable& aForwardingTable,
                                             const double     aAlpha,
                                             const double     aBeta)
    : LocalOptimizer(aForwardingTable)
    , theAlpha(aAlpha)
    , theBeta(aBeta)
    , theMutex()
    , theWeights() {
  LOG(INFO) << "Creating an async local optimizer for proportional fairness, "
               "with alpha = "
            << aAlpha << " and beta = " << aBeta;
}

void LocalOptimizerAsyncPF::operator()(const rpc::LambdaRequest& aReq,
                                       const std::string&        aDestination,
                                       const double              aTime) {
  ;
  assert(aTime > 0);
  const auto& myLambda                         = aReq.name();
  auto        myIstantaneousRate               = 1 / aTime;
  auto        myCurPrioritarizationCoefficient = 1;

  VLOG(2) << "req " << myLambda << ", dest " << aDestination
          << ", \"IstantaneousRate\" " << myIstantaneousRate << " Hz";

  const std::lock_guard<std::mutex> myLock(theMutex);

  /**
   * insert returns a pair:
   *    - first is the iterator to the newly inserted object or the iterator
   *      with the already present element with same key value
   *    - second is a boolean:
   *        + TRUE if the new element has beeen effectively inserted
   *        + FALSE if there was already an element with same key value
   *
   * the bool of the first insert is FALSE if there is already an element for
   * the aLambda the bool of the second one is FALSE if there is already
   * aDestination per aLambda
   *
   * TODO: need to verify if the element inserted will be scheduled (need to
   * verify if alpha and beta are enough to prevent staleness), if they are not
   * a solution could be to insert an Elem with double max_value in order to
   * make it surely scheduled for the next aLambdaRequest
   * >> the new inserted destination should enter in the system with a big
   * weight in order to be surely scheduled
   */
  const auto myRet = theWeights.insert(
      {myLambda,
       std::map<std::string, Elem>(
           {{aDestination, Elem{1, 1, myIstantaneousRate}}})});
  if (not myRet.second) {
    const auto myInnerRet = myRet.first->second.insert(
        {aDestination, Elem{1, 1, myIstantaneousRate}});
    if (not myInnerRet.second) {
      myInnerRet.first->second.theLambdaServedCount += 1;
      myInnerRet.first->second.theExperiencedRateSum += myIstantaneousRate;

      auto historicalMeanRate = myInnerRet.first->second.theExperiencedRateSum /
                                myInnerRet.first->second.theLambdaServedCount;

      myCurPrioritarizationCoefficient =
          pow(myIstantaneousRate, theAlpha) / pow(historicalMeanRate, theBeta);

      myInnerRet.first->second.thePrioritizationCoefficient =
          myCurPrioritarizationCoefficient;
    }
  }

  theForwardingTable.change(
      myLambda, aDestination, myCurPrioritarizationCoefficient);
} // namespace edge

} // namespace edge
} // namespace uiiit
