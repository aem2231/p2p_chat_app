#pragma once
#include "core/message.hpp"
#include "network/connection.hpp"
#include "network/peer.hpp"
#include "network/discovery.hpp"
#include <mutex>
#include <string>
#include <vector>
#include <memory>
#include <boost/asio.hpp>
#include <map>
#include <queue>
#include <utility>
#include <vector>

class App {
  private:
  Discovery discovery_;
  std::vector<std::shared_ptr<Peer>> peers_;
  int selected_index_;
  boost::asio::io_context &io_context_;
  std::map<std::shared_ptr<Peer>, std::shared_ptr<Connection>> connections_;
  std::map<std::shared_ptr<Peer>, std::vector<Message>> message_history_;
  std::queue<std::pair<std::shared_ptr<Peer>, Message>> incoming_messages_;
  mutable std::mutex message_queue_mutex_;
  std::shared_ptr<Connection> getConnection(std::shared_ptr<Peer> peer) const;
  std::thread listener_thread;
  std::mutex listener_mutex_;
  boost::asio::ip::tcp::acceptor acceptor_;
  bool listening_;
  void onMessageReceived(std::shared_ptr<Peer> from, const Message& msg);
  void listenerLoop();
  std::pair<std::string, std::string> parseHandshake(const std::string& handshake);

  public:
  App(boost::asio::io_context& io_ctx);

  // Peer managment
  const std::vector<std::shared_ptr<Peer>>& getPeers() const;

  // Selection management
  void selectPeer(int index);
  std::shared_ptr<Peer> getSelectedPeer() const;
  int getSelectedIndex() const;
  void connectToPeer(std::shared_ptr<Peer> peer);
  void disconnectFromPeer(std::shared_ptr<Peer> peer);
  bool isConnectedTo(std::shared_ptr<Peer> peer) const;
  void sendMessageToSelected(const std::string& text);
  std::vector<std::pair<std::shared_ptr<Peer>, Message>> pollIncomingMessages();
  const std::vector<Message>& getMessageHistory(std::shared_ptr<Peer> peer);
};
