#include "network/connection.hpp"
#include "core/message.hpp"
#include "network/peer.hpp"
#include <boost/asio/read_until.hpp>
#include <boost/asio/write.hpp>
#include <functional>
#include <iostream>
#include <istream>
#include <mutex>

constexpr unsigned short DEFAULT_PORT = 9000;
using boost::asio::ip::tcp;

Connection::Connection(
  std::shared_ptr<Peer> peer,
  boost::asio::io_context& io_ctx,
  std::function<void(const Message&)> message_callback,
  std::function<void()> on_disconnect_
) : peer_(peer),
  socket_(io_ctx),
  on_message_received_(message_callback),
  on_disconnect_(on_disconnect_),
  connected_(false) {};

Connection::Connection(
  std::shared_ptr<Peer> peer,
  std::function<void(const Message&)> message_callback,
  std::function<void()> on_disconnect,
  boost::asio::ip::tcp::socket socket
) : peer_(peer),
    socket_(std::move(socket)),
    on_message_received_(message_callback),
    on_disconnect_(on_disconnect),
    connected_(true) {
  receive_thread_ = std::thread(&Connection::receiveLoop, this);
}

Connection::~Connection() {
  disconnect();
};

void Connection::connect() {
  try {
    // create endpoint (peer)
    tcp::endpoint endpoint(peer_->getIpAddr(), DEFAULT_PORT);

    // connect to peer
    socket_.connect(endpoint);

    {
      std::lock_guard<std::mutex> lock(mutex_);
      connected_ = true;
    }

    // start recieve thread
    receive_thread_ = std::thread(&Connection::receiveLoop, this);

  } catch (const boost::system::system_error& e) {
    // connection failed
    // The caller (App::connectToPeer) will handle the cleanup.
    throw; // rethrow for caller to handle
  }
}

bool Connection::sendMessage(const Message& msg) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!connected_) {
      return false;
    }
  }

  try { // try send a message
    std::string data = msg.serialize();
    data+='\n';
    boost::asio::write(socket_, boost::asio::buffer(data));
    return true;
  } catch (const boost::system::system_error& e) {
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      if (!connected_) {
        return false;
      }
      connected_ = false;
    }
    if (on_disconnect_) {
      on_disconnect_();
    }
    return false;
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
      // the connection has died
      {
        const std::lock_guard<std::mutex> lock(mutex_);
        // check if another thread has already handled the disconnect.
        if (!connected_) {
          return;
        }
        connected_ = false;
      }

      // no longer holding our internal lock, it is safe to call the external callback.
      if (on_disconnect_) {
        on_disconnect_();
      }
    }
  };
}

void Connection::disconnect() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    connected_ = false;
  }
  socket_.close();
  if (receive_thread_.joinable()) {
    receive_thread_.join();
  }
}

bool Connection::isConnected() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return connected_;
}
