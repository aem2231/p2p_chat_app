#include "core/app.hpp"
#include "core/message.hpp"
#include "network/connection.hpp"
#include "network/peer.hpp"
#include <cstddef>
#include <istream>
#include <memory>
#include <iostream>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

constexpr unsigned short DEFAULT_PORT = 9000;

// The new, more robust constructor
App::App(boost::asio::io_context& io_ctx)
  : selected_index_(-1),
    io_context_(io_ctx),
    acceptor_(io_context_),
    listener_thread(std::thread(&App::listenerLoop, this)),
    listener_thread(std::thread(&App::listenerLoop, this)),
    listening_(true) {

  // configure acceptor
  acceptor_.open(boost::asio::ip::tcp::v4());
  acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
  acceptor_.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), DEFAULT_PORT));
  acceptor_.listen();

}

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
    this->onMessageReceived(peer, msg);
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
    std::cerr << "Connection failed: " << e.what() << std::endl;
    connections_.erase(peer);
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

void App::onMessageReceived(std::shared_ptr<Peer> from, const Message& msg) {
  {
    std::lock_guard<std::mutex> lock(message_queue_mutex_);
    message_history_[from].push_back(msg);
    incoming_messages_.push({from, msg});
  }
}

void App::sendMessageToSelected(const std::string& text) {
  auto peer = getSelectedPeer();
  auto connection = getConnection(peer);

  if (!peer || connection == nullptr || !connection->isConnected()) {
    return;
    // add auto connect later
  }

  auto message = Message("You", text);
  connection->sendMessage(message);
  message_history_[peer].push_back(message);
  incoming_messages_.push({peer, message});
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

std::pair<std::string, std::string> App::parseHandshake(const std::string& handshake) {
  size_t first_pipe = handshake.find("|");
  if (first_pipe == std::string::npos) {
    return {};
  }
  // check if magic word is present
  std::string magic_word = handshake.substr(0, first_pipe);
  if (magic_word != "HELLO") {
    return {};
  }

  // extract hostname
  size_t second_pipe = handshake.find("|", first_pipe+1);
  if (second_pipe == std::string::npos) {
    return {};
  }
  std::string handshake_hostname = handshake.substr(first_pipe+1, second_pipe - first_pipe - 1);

  // extract ip
  std::string handshake_ip = handshake.substr(second_pipe+1);

  return {handshake_hostname, handshake_ip};
}

void App::listenerLoop() {
  while (listening_) {
    boost::asio::ip::tcp::socket socket(io_context_);
    acceptor_.accept(socket);
    boost::asio::streambuf buffer;
    boost::asio::read_until(socket, buffer, "\n");
    std::istream is(&buffer);
    std::string data;
    std::getline(is, data);
    auto handshake_data = parseHandshake(data);
    for (const auto& peer : peers_) {
      if (peer->getHostname() == handshake_data.first &&
        peer->getIpAddr() == boost::asio::ip::make_address(handshake_data.second)){
          auto new_connection = std::make_shared<Connection>(
            peer,
            [this, peer](const Message& msg){this->onMessageReceived(peer, msg);},
            std::move(socket)
          );
          connections_[peer] = new_connection;
          break;
        }
    }
  }
}

void App::performInitialDiscovery() {
  discovery_.start();
}

void App::refreshPeers() {
  peers_ = discovery_.getPeers();
}
