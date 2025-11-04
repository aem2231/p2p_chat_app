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
  PeerList peer_list(&app);
  ChatWindow chat_window(&app);

  // Layout
  auto layout = ftxui::Container::Horizontal({
    peer_list.getComponent(),
    chat_window.getComponent()
  });

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  std::thread poller([&] {
    while (true) {
      if (!app.pollIncomingMessages().empty()) {
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
