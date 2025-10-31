#pragma once
#include "network/peer.hpp"
#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <mutex>
#include <string>


class Connection {
  private:
  // member variables

  void recieveLoop();

  public:
  Connection();

  ~Connection();

  void connect();
  void sendMessage(const std::string& text);
  void disconnect();
  void isConnected() const;

}
