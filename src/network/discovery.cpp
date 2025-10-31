#include "network/discovery.hpp"
#include "network/peer.hpp"
#include <vector>

std::vector<std::shared_ptr<Peer>> Discovery::getPeers() const {
  auto peer1 = std::make_shared<Peer>("eris","127.0.2.4");
  auto peer2 = std::make_shared<Peer>("root", "192.92.62.23");
  auto peer3 = std::make_shared<Peer>("prez", "120.31.119.1");

  std::vector<std::shared_ptr<Peer>> peers;
  peers.push_back(peer1);
  peers.push_back(peer2);
  peers.push_back(peer3);
  return peers;
}
