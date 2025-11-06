#include "ui/peer_list.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <memory>
#include <vector>

PeerList::PeerList(App* app, ftxui::Component chat_input)
  : app_(app), chat_input_(chat_input) {
    // extract hostnames
    auto peers = app_->getPeers();

    container_ = ftxui::Renderer([this] {
      auto peers = app_->getPeers();
      ftxui::Elements elements;

      // loop through peers and build a list of them
      for (size_t i = 0; i < peers.size(); ++i) {
        const auto& peer = peers[i];

        auto name_element = ftxui::text(peer->getHostname());

        if (app_->isConnectedTo(peer)) {
          name_element = ftxui::text("[●] " + peer->getHostname()) | ftxui::color(ftxui::Color::Green);
        } else if (app_->isConnectingTo(peer)) {
          name_element = ftxui::text("[~] " + peer->getHostname()) | ftxui::color(ftxui::Color::Yellow);
        } else {
          name_element = ftxui::text("[ ] " + peer->getHostname());
        }

        // apply a style when a peer is selected
        if (static_cast<int>(i) == app_->getSelectedIndex()) {
          name_element |= ftxui::inverted;
        }
        elements.push_back(name_element);
      }

      return ftxui::vbox({
        ftxui::text("Peers") | ftxui::bold | ftxui::center,
        ftxui::separator(),
        ftxui::vbox(elements),
      }) | ftxui::border | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 20);
    });


    container_ |= ftxui::CatchEvent([this](const ftxui::Event& event) {
      if (event == ftxui::Event::ArrowDown) {
        int current_index = app_->getSelectedIndex();
        int peer_count = app_->getPeers().size();

        if (peer_count == 0) {
          return true;
        }

        int new_index = (current_index + 1) % peer_count;
        app_->selectPeer(new_index);
        return true;
      }

    if (event == ftxui::Event::ArrowUp) {
      int current_index = app_->getSelectedIndex();
      int peer_count = app_->getPeers().size();

      if (peer_count == 0) {
        return true;
      }

      int new_index = (current_index - 1 + peer_count) % peer_count;
      app_->selectPeer(new_index);
      return true;
    }

    if (event == ftxui::Event::Return || event == ftxui::Event::ArrowRight) {
      if (auto selected = app_->getSelectedPeer()) {
        std::thread connector_thread([this, selected] {
          app_->connectToPeer(selected);
        });
        connector_thread.detach();
      }
      chat_input_->TakeFocus();
      return true;
    }
    return false;
    });
}

ftxui::Component PeerList::getComponent() {
  return container_;
}
