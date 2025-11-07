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
    while (true) {
      // unconditionally post a refresh event.
      // ftxui will only redraw the screen if the model has changed
      screen.PostEvent(ftxui::Event::Custom);

      using namespace std::chrono_literals;
      std::this_thread::sleep_for(100ms); // Poll 10 times per second
    }
  });
  poller.detach();

  screen.Loop(layout);

  return 0;
}
