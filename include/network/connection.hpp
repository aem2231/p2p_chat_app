#pragma once
#include "core/message.hpp"
#include "network/peer.hpp"
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>

class Connection {
  private:
  // member variables
  std::shared_ptr<Peer> peer_;
  boost::asio::io_context& io_context_;
  boost::asio::ip::tcp::socket socket_;
  std::thread receive_thread_;
  std::function<void(const Message&)> on_message_received_;
  bool connected_;
  mutable std::mutex mutex_;

  // background thread function
  // listens for incoming messages
  void receiveLoop();

  public:
  Connection(
    std::shared_ptr<Peer> peer,
    boost::asio::io_context& io_ctx,
    std::function<void(const Message&)> callback
  );

  ~Connection();

  void connect();
  void sendMessage(const Message& msg);
  void disconnect();
  bool isConnected() const;

};
