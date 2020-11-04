#include "forwardingtablefactory.h"

#include "Support/conf.h"
#include "forwardingtable.h"

namespace uiiit {
namespace edge {

ForwardingTable* ForwardingTableFactory::make(const support::Conf& aConf) {

  const auto myType = aConf("type");

  if (myType == "random" || myType == "round-robin" ||
      myType == "least-impedance") {
    return new ForwardingTable(forwardingTableTypeFromString(aConf("type")));

  } else if (myType == "proportional-fairness") {
    return new ForwardingTable(forwardingTableTypeFromString(aConf("type")),
                            aConf.getDouble("alpha"),
                            aConf.getDouble("beta"));
  } else {
    throw std::runtime_error("Invalid forwarding table type: " + myType);
  }
  return nullptr;
}

} // namespace edge
} // namespace uiiit
