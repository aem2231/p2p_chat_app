#include "ui/chat_window.hpp"
#include "core/app.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/terminal.hpp>
#include <vector>

ChatWindow::ChatWindow(App* app)
  : app_(app),
  input_text_("") {
    input_component_ = ftxui::Input(&input_text_, "Type here..");

    // event handler
    input_component_ |= ftxui::CatchEvent([this](const ftxui::Event& event) {
      if (event == ftxui::Event::Return && input_text_ != "") {
        app_->sendMessageToSelected(input_text_);
        input_text_.clear();
        return true;
      }

      if (event == ftxui::Event::ArrowLeft) {
        peer_list_->TakeFocus();
        return true;
      }
      return false;
    });

    container_ = ftxui::Renderer(input_component_, [this]() {
      ftxui::Elements elements;

      auto selected = app_->getSelectedPeer();
      auto status = app_->getStatusMessage();

      // set title to hostname of peer
      auto title = ftxui::text(selected ?
        selected->getHostname() :
        "No user selected") | ftxui::bold | ftxui::center;

      auto status_display = ftxui::text(status);
      status_display |= ftxui::color(ftxui::Color::Red);

      // messages area
      auto messages = ftxui::text("Messages will appear here") | ftxui::dim;
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

      // add elements to vector
      elements.push_back(title);
      elements.push_back(ftxui::separator());
      elements.push_back(ftxui::text("") | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, static_cast<float>(0.5)));  // spacing
      elements.push_back(messages_display | ftxui::flex);
      elements.push_back(ftxui::text("") | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 1));
      if (status != "") {
        elements.push_back(ftxui::separator());
        elements.push_back(status_display);
      }
      elements.push_back(ftxui::separator());
      elements.push_back(input_display);

      // layout
      return ftxui::vbox({
        elements
      }) | ftxui::border | size(ftxui::WIDTH, ftxui::EQUAL, static_cast<int>(ftxui::Terminal::Size().dimx) - 20);

    });
}

ftxui::Component ChatWindow::getComponent() {
  return container_;
}

// setter to link peer_list
void ChatWindow::setPeerListComponent(ftxui::Component peer_list) {
  peer_list_ = peer_list;
}
