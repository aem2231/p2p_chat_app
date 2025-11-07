#include "core/app.hpp"
#include "ui/peer_list.hpp"
#include "ui/chat_window.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

int main() {
  boost::asio::io_context io_context;
  App app(io_context);

  // start the discovery service and get the initial list of peers
  app.performInitialDiscovery();
  app.refreshPeers();

  ChatWindow chat_window(&app);
  PeerList peer_list(&app, chat_window.getInputComponent());

  // Layout
  auto layout = ftxui::Container::Horizontal({
    peer_list.getComponent(),
    chat_window.getComponent()
  });

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  std::thread poller([&] {
    auto last_peer_refresh = std::chrono::steady_clock::now();
    std::string last_status_message;

    while (true) {
      bool should_refresh_ui = false;

      // check new messages
      if (!app.pollIncomingMessages().empty()) {
        should_refresh_ui = true;
      }

      // peer list refresh (every 3 seconds)
      auto now = std::chrono::steady_clock::now();
      if (now - last_peer_refresh > std::chrono::seconds(3)) {
        app.refreshPeers();
        last_peer_refresh = now;
        should_refresh_ui = true;
      }

      // check status message change
      std::string current_status = app.getStatusMessage();
      if (current_status != last_status_message) {
        last_status_message = current_status;
        should_refresh_ui = true;
      }

      // if anything changed, post a single refresh event
      if (should_refresh_ui) {
        screen.PostEvent(ftxui::Event::Custom);
      }

      using namespace std::chrono_literals;
      std::this_thread::sleep_for(100ms);
    }
  });
  poller.detach();

  screen.Loop(layout);

  return 0;
}
