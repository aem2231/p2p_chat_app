#include "network/connection.hpp"
#include "core/message.hpp"
#include "network/peer.hpp"
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <iostream>
#include <istream>
#include <mutex>

constexpr unsigned short DEFAULT_PORT = 9000;
using boost::asio::ip::tcp;

Connection::Connection(
  std::shared_ptr<Peer> peer,
  boost::asio::io_context& io_ctx,
  std::function<void(const Message&)> callback
) : peer_(peer),
  io_context_(io_ctx),
  socket_(io_context_),
  on_message_received_(callback),
  connected_(false) {};

Connection::~Connection() {
  disconnect();
};

void Connection::connect() {
  try {
    // create endpoint (peer)
    tcp::endpoint endpoint(peer_->getIpAddr(), DEFAULT_PORT);

    // connect to peer
    socket_.connect(endpoint);

    std::lock_guard<std::mutex> lock(mutex_);
    connected_ = true;

    // start recieve thread
    receive_thread_ = std::thread(&Connection::receiveLoop, this);

  } catch (const boost::system::system_error& e) {
    // connection failed
    std::cerr << "Connection failed: " << e.what() << std::endl;
    throw; // rethrow for caller to handle
  }
}

void Connection::sendMessage(const Message& msg) {
  if (!connected_) {
    return;
  }

  try { // try send a message
    std::string data = msg.serialize();
    data+='\n';
    boost::asio::write(socket_, boost::asio::buffer(data));
  } catch (const boost::system::system_error& e) {
    std::cerr << "Failed to send message: " << e.what() << std::endl;
    std::lock_guard<std::mutex> lock(mutex_);
    connected_ = false;
  }
}

void Connection::receiveLoop() {
  boost::asio::streambuf buffer;
  while(connected_) {
    try { // listen for incoming messages while connected
      boost::asio::read_until(socket_, buffer, "\n");
      std::istream is(&buffer); // place the buffer into an input stream
      std::string data;
      std::getline(is, data); // remove the delimiter
      on_message_received_(Message::deserialize(data));

    } catch (const boost::system::system_error& e) {
      std::cerr << "Something went wrong: " << e.what() << std::endl;
      std::lock_guard<std::mutex> lock(mutex_);
      connected_ = false;
    }
  };
}

void Connection::disconnect() {
  std::lock_guard<std::mutex> lock(mutex_);
  connected_ = false;
  socket_.close();
  if (receive_thread_.joinable()) {
    receive_thread_.join();
  }
}

bool Connection::isConnected() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return connected_;
}
