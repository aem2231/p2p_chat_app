#include "ui/peer_list.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <vector>

PeerList::PeerList(App* app)
  : app_(app),
  selected_(0) {
    // extract hostnames
    auto peers = app_->getPeers();
    for (const auto& peer: peers) {
      peer_names_.push_back(peer->getHostname());
    }

    // create menu
    menu_component_ = ftxui::Menu(
      &peer_names_,
      &selected_
    );

    // event handler
    menu_component_ |= ftxui::CatchEvent([this](ftxui::Event event) {
      if (event == ftxui::Event::Return) {
        app_->selectPeer(selected_);
        return true;
      }
      return false;
    });
}

ftxui::Component PeerList::getComponent() {
  // wrap peer list with centered content
  return Renderer(menu_component_, [&] {
    return ftxui::vbox({
      ftxui::text("Peers") | ftxui::bold | ftxui::center,
      ftxui::separator(),
      menu_component_->Render() | ftxui::center
    }) | ftxui::border | size(ftxui::WIDTH, ftxui::EQUAL, 20);
  });
}
