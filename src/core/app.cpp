#include "core/app.hpp"
#include "core/message.hpp"
#include "network/connection.hpp"
#include "network/peer.hpp"
#include <memory>
#include <iostream>
#include <mutex>
#include <utility>
#include <vector>

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

std::shared_ptr<Connection> App::getConnection(std::shared_ptr<Peer> peer) const {
  auto connection = connections_.find(peer);
  if (connection == connections_.end()) {
    return nullptr;
  }
  return connection->second;
}

void App::onMessageRecieved(std::shared_ptr<Peer> from, const Message& msg) {
  {
    std::lock_guard<std::mutex> lock(message_queue_mutex_);
    message_history_[from].push_back(msg);
    incoming_messages_.push({from, msg});
  }
}

void App::sendMessageToSelected(const std::string& text){
  auto peer = getSelectedPeer();
  auto connection = getConnection(peer);

  if (!peer || connection == nullptr || !connection->isConnected()) {
    return;
    // add auto connect later
  }

  auto message = Message("You", text);
  connection->sendMessage(message);
  message_history_[peer].push_back(message);
}

std::vector<std::pair<std::shared_ptr<Peer>, Message>> App::pollIncomingMessages() {
  {
    std::lock_guard<std::mutex> lock(message_queue_mutex_);
    std::vector<std::pair<std::shared_ptr<Peer>, Message>> messages;
    while (!incoming_messages_.empty()) {
      messages.push_back(incoming_messages_.front());
      incoming_messages_.pop();
    }
    return messages;
  }
}

const std::vector<Message>& App::getMessageHistory(std::shared_ptr<Peer> peer) {
  auto history = message_history_.find(peer);
  if (history == message_history_.end()) {
    static const std::vector<Message> empty_history;
    return empty_history;
  }
  return history->second;
}
