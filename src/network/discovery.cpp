#include "network/discovery.hpp"
#include "network/peer.hpp"
#include <memory>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include <mutex>
#include <iostream>
#include <chrono>

const unsigned short DISCOVERY_PORT = 9001;

Discovery::Discovery()
  : broadcast_socket_(io_context_),
  receive_socket_(io_context_) {}

Discovery::~Discovery() {
  this->stop();
}

void Discovery::start() {
  this->running_ = true;

  broadcast_thread_ = std::thread(&Discovery::broadcastLoop, this);
  broadcast_thread_.detach();

  receive_thread_ = std::thread(&Discovery::receiveLoop, this);
  receive_thread_.detach();
}

void Discovery::stop() {
  running_ = false;
  receive_socket_.close();
  broadcast_socket_.close();
  if (broadcast_thread_.joinable()) {
    broadcast_thread_.join();
  }

  if (receive_thread_.joinable()) {
    receive_thread_.join();
  }
}

void Discovery::addPeer(const std::shared_ptr<Peer>& new_peer) {
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
  receive_socket_.open(boost::asio::ip::udp::v4());
  receive_socket_.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), DISCOVERY_PORT));

  while (running_) {
    try {
      std::array<char, 1024> recv_buffer;
      boost::asio::ip::udp::endpoint sender_endpoint;

      size_t len = receive_socket_.receive_from(
        boost::asio::buffer(recv_buffer), sender_endpoint
      );

      //  convert data to string
      std::string message(recv_buffer.data(), len);

      // check message type
      if (message.rfind("P2P_PONG|", 0) == 0) {
        // find first pipe and extract hostname
        size_t first_pipe = message.find("|");
        if (first_pipe != std::string::npos) {
          std::string hostname = message.substr(first_pipe + 1);
          std::string ip_addr = sender_endpoint.address().to_string();
          auto new_peer = std::make_shared<Peer>(hostname, ip_addr);
          addPeer(new_peer);
        }
      } else if (message == "P2P_PING") {
        std::string hostname = "test";
        std::string response_message = "P2P_PONG|" + hostname;

        broadcast_socket_.send_to(
          boost::asio::buffer(response_message),
          sender_endpoint
        );
      }

    } catch (const boost::system::system_error& e) {
      std::cerr << "Recieve loop network error: " << e.what() << std::endl;
    }
  }
}

void Discovery::broadcastLoop () {
  using namespace std::chrono_literals;

  // create an endpoint representing al devices on a network
  auto broadcast_address = boost::asio::ip::address_v4::broadcast();
  boost::asio::ip::udp::endpoint broadcast_endpoint(broadcast_address, DISCOVERY_PORT);

  // open socket for ipv4 udp
  broadcast_socket_.open(boost::asio::ip::udp::v4());
  broadcast_socket_.set_option(boost::asio::socket_base::broadcast(true));

  std::string message = "P2P_PING";

  while (running_) {
    try {
      broadcast_socket_.send_to(boost::asio::buffer(message), broadcast_endpoint);
      std::this_thread::sleep_for(3s);
    } catch (const boost::system::system_error& e) {
      std::cerr << "Broadcast loop network error: " << e.what() << std::endl;
    }
  }
}

std::vector<std::shared_ptr<Peer>> Discovery::getPeers() const {
  const std::lock_guard<std::mutex> lock(peers_mutex_);
  return discovered_peers_;
}
