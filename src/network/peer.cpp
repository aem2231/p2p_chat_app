#include "network/peer.hpp"
#include <boost/asio/ip/address.hpp>

Peer::Peer(const std::string& hostname, const std::string& ip_str)
  : hostname_(hostname),
  ip_addr_(boost::asio::ip::make_address(ip_str)) {
}

// Getter: return reference to hostname
const std::string& Peer::getHostname() const {
  return hostname_;
}

// Getter: return IP address by value
boost::asio::ip::address Peer::getIpAddr() const {
  return ip_addr_;
}
