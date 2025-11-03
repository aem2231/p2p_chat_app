#include "ui/chat_window.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

ChatWindow::ChatWindow(App* app)
  : app_(app),
  input_text_("") {
    input_component_ = ftxui::Input(&input_text_, "Type here..");
    container_ = ftxui::Renderer(input_component_, [this]() {
      auto selected = app_->getSelectedPeer();

      // set title to hostname of peer
      auto title = ftxui::text(selected ?
        selected->getHostname() :
        "No user selected") | ftxui::bold | ftxui::center;

      // messages area
      auto messages = ftxui::text("Messages will appear here") | ftxui::dim;

      // input
      auto input_display = ftxui::hbox({
        ftxui::text("> "),
        input_component_->Render()
      });

      // layout
      return ftxui::vbox({
        title,
        ftxui::separator(),
        ftxui::text("") | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, static_cast<float>(0.5)),  // spacing
        messages | ftxui::flex,
        ftxui::text("") | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 1),
        ftxui::separator(),
        input_display
      }) | ftxui::border | size(ftxui::WIDTH, ftxui::EQUAL, static_cast<int>(ftxui::Terminal::Size().dimx) - 20);

    });
}

ftxui::Component ChatWindow::getComponent() {
  return container_;
}
