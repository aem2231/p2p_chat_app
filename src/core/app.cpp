#include "core/app.hpp"
#include "core/message.hpp"
#include "network/connection.hpp"
#include "network/peer.hpp"
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

constexpr unsigned short DEFAULT_PORT = 9000;

App::App(boost::asio::io_context& io_ctx)

  : my_hostname_("unknown"),
    selected_index_(-1),
    io_context_(io_ctx),
    acceptor_(io_context_),
    listener_thread(std::thread(&App::listenerLoop, this)),
    listening_(true) {

  try {
    my_hostname_ = boost::asio::ip::host_name();
  } catch (const boost::system::system_error&) {
    // fallback to "unknown"
  }

  acceptor_.open(boost::asio::ip::tcp::v4());
  acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
  acceptor_.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), DEFAULT_PORT));
  acceptor_.listen();
}

App::~App() {
  stop();
}

void App::stop() {
  listening_ = false;
  acceptor_.close();
  if (listener_thread.joinable()) {
    listener_thread.join();
  }
  discovery_.stop();
  const std::lock_guard<std::mutex> lock(connector_thread_mutex_);
  for (auto& thread : connector_threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
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
  const std::lock_guard<std::mutex> lock(connector_thread_mutex_);
  connector_threads_.emplace_back([this, peer] {
    // check if we are connecting to a peer to prevent double connections
    {
      const std::lock_guard<std::mutex> lock(connecting_peers_mutex_);
      if (connecting_peers_.count(peer)) {
        return;
      }
      connecting_peers_.insert(peer);
    }

    // check if the peer is valid
    if (!peer) {
      const std::lock_guard<std::mutex> lock(connecting_peers_mutex_);
      connecting_peers_.erase(peer);
      return;
    }

    if (App::isConnectedTo(peer)) {
      const std::lock_guard<std::mutex> lock(connecting_peers_mutex_);
      connecting_peers_.erase(peer);
      return;
    }

    auto on_message_callback = [this, peer](const Message& msg) {
      this->onMessageReceived(peer, msg);
    };

    auto on_disconnect_callback = [this, peer] {
      std::shared_ptr<Connection> old_connection;
      {
        const std::lock_guard<std::mutex> lock(app_mutex_);
        auto it = connections_.find(peer);
        if (it != connections_.end()) {
          old_connection = it->second;
          connections_.erase(it);
        }
        status_message_ = "Connection to " + peer->getHostname() + " lost.";
      }
    };

    auto new_connection = std::make_shared<Connection>(
      peer,
      io_context_,
      on_message_callback,
      on_disconnect_callback
    );

    {
      const std::lock_guard<std::mutex> lock(app_mutex_);
      connections_.insert({peer, new_connection});
    }

    try {
      new_connection->connect();
      status_message_ = "";
    } catch (const std::exception& e) {
      const std::lock_guard<std::mutex> lock(app_mutex_);
      connections_.erase(peer);
      status_message_ = "Failed to connect to " + peer->getHostname();
    }

    {
      const std::lock_guard<std::mutex> lock(connecting_peers_mutex_);
      connecting_peers_.erase(peer);
    }
  });
}

bool App::isConnectedTo(std::shared_ptr<Peer> peer) const {
  const std::lock_guard<std::mutex> lock(app_mutex_);
  auto connection = connections_.find(peer);
  if (connection == connections_.end()) {
    return false; // no entry found
  }

  auto connection_ptr = connection->second;
  return connection_ptr->isConnected();
}

bool App::isConnectingTo(std::shared_ptr<Peer> peer) const {
  const std::lock_guard<std::mutex> lock(connecting_peers_mutex_);
  return connecting_peers_.count(peer);
}

std::shared_ptr<Connection> App::getConnection(std::shared_ptr<Peer> peer) const {
  const std::lock_guard<std::mutex> lock(app_mutex_);
  auto connection = connections_.find(peer);
  if (connection == connections_.end()) {
    return nullptr;
  }
  return connection->second;
}

void App::onMessageReceived(std::shared_ptr<Peer> from, const Message& msg) {
  {
    const std::lock_guard<std::mutex> lock(message_queue_mutex_);
    message_history_[from].push_back(msg);
    incoming_messages_.push({from, msg});
  }
}

void App::sendMessageToSelected(const std::string& text) {
  auto peer = getSelectedPeer();
  if (!peer) {
    return;
  }

  std::shared_ptr<Connection> connection;
  {
    const std::lock_guard<std::mutex> lock(app_mutex_);
    auto it = connections_.find(peer);
    if (it != connections_.end()) {
        connection = it->second;
    }
  }

  if (connection == nullptr || !connection->isConnected()) {
    connectToPeer(peer); // auto-connect on send
    return;
  }

  // Create the message with our hostname to send over the network
  auto message_to_send = Message(my_hostname_, text);
  if (!connection->sendMessage(message_to_send)) {
    const std::lock_guard<std::mutex> lock(app_mutex_);
    status_message_ = "Failed to send message.";
  } else {
    const std::lock_guard<std::mutex> lock(message_queue_mutex_);
    auto message_for_history = Message("You", text);
    message_history_[peer].push_back(message_for_history);
    incoming_messages_.push({peer, message_for_history});
  }
}

std::vector<std::pair<std::shared_ptr<Peer>, Message>> App::pollIncomingMessages() {
  {
    const std::lock_guard<std::mutex> lock(message_queue_mutex_);
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

void App::listenerLoop() {
  while (listening_) {
    try {
      boost::asio::ip::tcp::socket socket(io_context_);
      acceptor_.accept(socket);

      auto remote_ip = socket.remote_endpoint().address();
      std::shared_ptr<Peer> connected_peer = nullptr;

      for (const auto& peer : peers_) {
        if (peer->getIpAddr() == remote_ip) {
          connected_peer = peer;
          break;
        }
      }

      if (connected_peer) {
        auto on_disconnect_callback = [this, connected_peer] {
          std::shared_ptr<Connection> old_connection;
          {
            const std::lock_guard<std::mutex> lock(app_mutex_);
            auto it = connections_.find(connected_peer);
            if (it != connections_.end()) {
              old_connection = it->second;
              connections_.erase(it);
            }
            if (connected_peer) {
              status_message_ = "Connection to " + connected_peer->getHostname() + " lost.";
            }
          }
        };

        auto on_message_callback = [this, connected_peer](const Message& msg) {
          this->onMessageReceived(connected_peer, msg);
        };

        auto new_connection = std::make_shared<Connection>(
          connected_peer,
          on_message_callback,
          on_disconnect_callback,
          std::move(socket)
        );
        {
            std::shared_ptr<Connection> old_connection;
            const std::lock_guard<std::mutex> lock(app_mutex_);
            auto it = connections_.find(connected_peer);
            if (it != connections_.end()) {
                old_connection = it->second;
                it->second = new_connection;
            } else {
                connections_.insert({connected_peer, new_connection});
                status_message_ = "";
            }
        }
      } else {
        socket.close();
      }
    } catch (const boost::system::system_error& e) {
      // Errors are expected here if the acceptor is closed.
    }
  }
}

void App::performInitialDiscovery() {
  discovery_.start();
}

void App::refreshPeers() {
  peers_ = discovery_.getPeers();
}

//const std::string& App::getStatusMessage() const {
std::string App::getStatusMessage() const {
  const std::lock_guard<std::mutex> lock(app_mutex_);
  return status_message_;
}
