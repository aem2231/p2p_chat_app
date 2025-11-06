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

    while (true) {
      bool should_refresh_ui = false;

      // check for new messages
      if (!app.pollIncomingMessages().empty()) {
        should_refresh_ui = true;
      }

      // refresh peer list every 3s
      auto now = std::chrono::steady_clock::now();
      if (now - last_peer_refresh > std::chrono::seconds(3)) {
        app.refreshPeers();
        last_peer_refresh = now;
        should_refresh_ui = true;
      }

      // refresh ui if needed
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
