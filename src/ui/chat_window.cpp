#include "ui/chat_window.hpp"
#include "core/app.hpp"
#include "network/peer.hpp"
#include <cstddef>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/terminal.hpp>
#include <vector>
#include <algorithm>

ChatWindow::ChatWindow(App* app)
  : app_(app),
  input_text_("") {
    input_component_ = ftxui::Input(&input_text_, "Type here..");

    // event handler
    input_component_ |= ftxui::CatchEvent([this](ftxui::Event event) {
      if (event == ftxui::Event::Return && input_text_ != "") {
        app_->sendMessageToSelected(input_text_);
        input_text_.clear();
        return true;
      }
      return false;
    });

    container_ = ftxui::Renderer(input_component_, [this]() {
      auto selected = app_->getSelectedPeer();

      // set title to hostname of peer
      auto title = ftxui::text(selected ?
        selected->getHostname() :
        "No user selected") | ftxui::bold | ftxui::center;

      // messages area
      //auto messages = ftxui::text("Messages will appear here") | ftxui::dim;
      ftxui::Element messages_display;
      const auto& message_history = app_->getMessageHistory(selected);

      if (!message_history.empty()){
        ftxui::Elements message_elements;
        for (const auto& msg : message_history) {
          auto sender_element = ftxui::text(msg.sender) | ftxui::bold;
          auto content_element = ftxui::text(": " + msg.content);
          auto time_elemenet = ftxui::text(" [" + msg.getFormattedTime() + "]") | ftxui::color(ftxui::Color(ftxui::Color::GrayDark));

          if (msg.sender == "You") {
            sender_element |= ftxui::color(ftxui::Color::Green);
          } else {
            sender_element |= ftxui::color(ftxui::Color::Cyan);
          }

          auto line = ftxui::hbox({
            sender_element,
            content_element,
            time_elemenet,
          });
          message_elements.push_back(line);
        }
        messages_display = ftxui::vbox(message_elements);
      } else {
        messages_display = ftxui::text("No messages yet.") | ftxui::center | ftxui::dim;
      }

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
        messages_display | ftxui::flex,
        ftxui::text("") | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 1),
        ftxui::separator(),
        input_display
      }) | ftxui::border | size(ftxui::WIDTH, ftxui::EQUAL, static_cast<int>(ftxui::Terminal::Size().dimx) - 20);

    });
}

ftxui::Component ChatWindow::getComponent() {
  return container_;
}
