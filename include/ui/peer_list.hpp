#pragma once
#include "core/app.hpp"
#include <ftxui/component/component.hpp>
#include <vector>
#include <string>

class PeerList {
  private:
  App* app_;
  std::vector<std::string> peer_names_;
  int selected_;
  ftxui::Component menu_component_;

  public:
  PeerList(App* app);

  ftxui::Component getComponent();
};
