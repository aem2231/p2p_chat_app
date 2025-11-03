#include "core/app.hpp"
#include "ui/peer_list.hpp"
#include "ui/chat_window.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <boost/asio.hpp>

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
  screen.Loop(layout);

  return 0;
}