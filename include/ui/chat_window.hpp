#pragma once
#include "core/app.hpp"
#include <ftxui/component/component.hpp>
#include <string>

class ChatWindow{
  private:
  App* app_;
  std::string input_text_;
  ftxui::Component input_component_;
  ftxui::Component container_;

  public:
  ChatWindow(App* app);
  ftxui::Component getInputComponent() { return input_component_; }
  ftxui::Component getComponent();
};
