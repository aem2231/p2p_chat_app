#pragma once
#include "network/peer.hpp"
#include "network/discovery.hpp"
#include <vector>
#include <memory>

class App {
  private:
  Discovery discovery_;
  std::vector<std::shared_ptr<Peer>> peers_;
  int selected_index_;

  public:
  App();

  // Peer managment
  const std::vector<std::shared_ptr<Peer>>& getPeers() const;

  // Selection management
  void selectPeer(int index);
  std::shared_ptr<Peer> getSelectedPeer() const;
  int getSelectedIndex() const;
};
