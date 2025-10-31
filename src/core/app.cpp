#include "core/app.hpp"
#include <memory>

App::App()
  : selected_index_(-1) {
  peers_ = discovery_.getPeers();
};

const std::vector<std::shared_ptr<Peer>>& App::getPeers() const {
  return peers_;
}

// checks that an index is within the bounds of the peers vector
void App::selectPeer(int index) {
  if (index < 0 ||static_cast<size_t>(index) >= peers_.size()) {
    selected_index_ = -1;
    return;
  }

  selected_index_ = index;
}

// returns a pointer to the selected peer object
std::shared_ptr<Peer> App::getSelectedPeer() const {
  if (selected_index_ ==  -1){
    return nullptr;
  }
  return peers_[selected_index_ ];
}

int App::getSelectedIndex() const {
  return selected_index_;
}
