#pragma once
#include <string>
#include <chrono>

class Message {
  public:
  std::string sender;
  std::string content;
  std::chrono::system_clock::time_point timestamp;

  Message(const std::string& sender, const std::string& content); // create new message
  Message() = default;

  // serialization
  std::string serialize() const;
  static Message deserialize(const std::string& data);

  // helpers
  std::string getFormattedTime() const;
};
