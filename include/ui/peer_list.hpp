#pragma once
#include "core/app.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>

class PeerList {
  private:
  App* app_;
  ftxui::Component container_;
  ftxui::Component chat_input_;

  public:
  PeerList(App* app, ftxui::Component chat_input);
  ftxui::Component getComponent();
};
