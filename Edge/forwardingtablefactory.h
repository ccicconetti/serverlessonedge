#pragma once

#include <memory>

namespace uiiit {

namespace support {
class Conf;
}

namespace edge {

class ForwardingTable;

class ForwardingTableFactory final
{
 public:
  static ForwardingTable* make(const support::Conf& aConf);
};

} // namespace edge
} // namespace uiiit