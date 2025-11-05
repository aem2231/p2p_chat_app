#pragma once
#include "network/peer.hpp"
#include <vector>
#include <memory>
#include <boost/asio.hpp>
#include <mutex>
#include <thread>
#include <chrono>

class Discovery{
  private:
  boost::asio::io_context io_context_;

  boost::asio::ip::udp::socket broadcast_socket_;
  boost::asio::ip::udp::socket receive_socket_;

  std::thread broadcast_thread_;
  std::thread receive_thread_;

  mutable std::mutex peers_mutex_;
  std::vector<std::shared_ptr<Peer>> discovered_peers_;

  bool running_ = false;

  void receiveLoop();
  void broadcastLoop();

  public:
  Discovery();
  ~Discovery();
  void start();
  void stop();
  void addPeer(const std::shared_ptr<Peer>& new_peer);
  std::vector<std::shared_ptr<Peer>> getPeers() const;
};
