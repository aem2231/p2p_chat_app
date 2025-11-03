#include "core/app.hpp"
#include "core/message.hpp"
#include "network/connection.hpp"
#include "network/peer.hpp"
#include <memory>
#include <iostream>
#include <mutex>

App::App(boost::asio::io_context& io_ctx)
  : selected_index_(-1),
    io_context_(io_ctx) {
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

void App::connectToPeer(std::shared_ptr<Peer> peer) {
  // check if the peer is valid
  if (!peer) { return; }

  if (App::isConnectedTo(peer)) {
    return;
  }

  // define a callback that Connection executes in a background thread
  // captures 'this'  to call app's handler and 'peer' to indetify the sender
  auto callback = [this, peer](const Message& msg) {
    this->onMessageRecieved(peer, msg);
  };

  // create a new connection
  auto new_connection = std::make_shared<Connection>(
    peer,
    io_context_,
    callback
  );

  // add the new connection to the connections map
  connections_.insert({peer, new_connection});
  try {
    new_connection->connect();
  } catch (const std::exception& e) {
    std::cerr << "Something went wrong: " << e.what() << std::endl;
    connections_.erase(peer);
    throw;
  }
}

bool App::isConnectedTo(std::shared_ptr<Peer> peer) const {
  auto connection = connections_.find(peer);
  if (connection == connections_.end()) {
    return false; // no entry found
  }

  auto connection_ptr = connection->second;
  return connection_ptr->isConnected();
}

void App::onMessageRecieved(std::shared_ptr<Peer> from, const Message& msg) {
  {
    std::lock_guard<std::mutex> lock(message_queue_mutex_);
    message_history_[from].push_back(msg);
    incoming_messages_.push({from, msg});
  }


}
