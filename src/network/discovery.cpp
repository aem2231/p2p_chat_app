#include "network/discovery.hpp"
#include "network/peer.hpp"
#include <memory>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include <mutex>

constexpr unsigned short DISCOVERY_PORT = 9001;
constexpr const char* PING_MESSAGE = "P2P_PING";

Discovery::Discovery()
  : socket_(io_context_) {
}

Discovery::~Discovery() {
  stop();
}

void Discovery::start() {
  try {
    socket_.open(boost::asio::ip::udp::v4());
    socket_.set_option(boost::asio::socket_base::reuse_address(true));
    socket_.set_option(boost::asio::socket_base::broadcast(true));
    socket_.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), DISCOVERY_PORT));
  } catch (const boost::system::system_error& e) {
    return;
  }

  running_ = true;
  broadcast_thread_ = std::thread(&Discovery::broadcastLoop, this);
  receive_thread_ = std::thread(&Discovery::receiveLoop, this);
}

void Discovery::stop() {
  running_ = false;
  boost::system::error_code ec;
  socket_.close(ec);
  if (broadcast_thread_.joinable()) {
    broadcast_thread_.join();
  }
  if (receive_thread_.joinable()) {
    receive_thread_.join();
  }
}

void Discovery::addPeer(const std::shared_ptr<Peer>& new_peer) {
  // don't add our own device to the peer list
  try {
    if (new_peer->getHostname() == boost::asio::ip::host_name()) {
      return;
    }
  } catch (const boost::system::system_error&) {
    // failed to get local hostname
  }

  const std::lock_guard<std::mutex> lock(peers_mutex_);

  bool exists = false;
  for (const auto& existing_peer : discovered_peers_) {
    if (existing_peer->getIpAddr() == new_peer->getIpAddr()) {
      exists = true;
      break;
    }
  }
  if (!exists) {
    discovered_peers_.push_back(new_peer);
  }
}

void Discovery::receiveLoop() {
  while (running_) {
    try {
      std::array<char, 1024> recv_buffer;
      boost::asio::ip::udp::endpoint sender_endpoint;

      size_t len = socket_.receive_from(
        boost::asio::buffer(recv_buffer), sender_endpoint
      );

      std::string message(recv_buffer.data(), len);
      if (message.rfind("P2P_PONG|", 0) == 0) {
        size_t first_pipe = message.find("|");
        if (first_pipe != std::string::npos) {
          std::string hostname = message.substr(first_pipe + 1);
          std::string ip_addr = sender_endpoint.address().to_string();
          auto new_peer = std::make_shared<Peer>(hostname, ip_addr);
          addPeer(new_peer);
        }
      } else if (message == PING_MESSAGE) {
        std::string hostname = "unknown_host";
        try {
          hostname = boost::asio::ip::host_name();
        } catch (const boost::system::system_error&) {}

        std::string response_message = "P2P_PONG|" + hostname;

        socket_.send_to(
          boost::asio::buffer(response_message),
          sender_endpoint
        );
      }
    } catch (const boost::system::system_error& e) {
      // Errors are expected here when the socket is closed.
    }
  }
}

void Discovery::broadcastLoop () {
  using namespace std::chrono_literals;

  auto broadcast_address = boost::asio::ip::address_v4::broadcast();
  boost::asio::ip::udp::endpoint broadcast_endpoint(broadcast_address, DISCOVERY_PORT);
  std::string message = PING_MESSAGE;

  while (running_) {
    try {
      socket_.send_to(boost::asio::buffer(message), broadcast_endpoint);
      std::this_thread::sleep_for(3s);
    } catch (const boost::system::system_error& e) {
      // Errors are expected here when the socket is closed.
    }
  }
}

std::vector<std::shared_ptr<Peer>> Discovery::getPeers() const {
  const std::lock_guard<std::mutex> lock(peers_mutex_);
  return discovered_peers_;
}
