#pragma once
#include "network/peer.hpp"
#include <vector>
#include <memory>

class Discovery{
  public:
  std::vector<std::shared_ptr<Peer>> getPeers() const;
};
