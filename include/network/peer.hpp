#pragma once
#include <string>
#include <boost/asio/ip/address.hpp>

class Peer {
private:
  std::string hostname_;
  boost::asio::ip::address ip_addr_;

public:
  Peer(const std::string& hostname, const std::string& ip_str);

  // Getters
  const std::string& getHostname() const;
  boost::asio::ip::address getIpAddr() const;
};
