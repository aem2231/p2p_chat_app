#include "core/message.hpp"
#include <chrono>
#include <sstream>
#include <ctime>
#include <string>

Message::Message(const std::string& sender, const std::string& content)
  : sender(sender),
    content(content),
    timestamp(std::chrono::system_clock::now()) {}

std::string Message::serialize() const {
  // convert timestamp to str
  auto since_epoch = timestamp.time_since_epoch();
  auto ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
  long long timestamp_value = ms_since_epoch.count();

  // build string to send
  std::ostringstream oss;
  oss << sender << "|" << timestamp_value << "|" << content;
  return oss.str();
}

Message Message::deserialize(const std::string& data) {
  // extract sender
  size_t first_pipe = data.find("|");
  std::string sender_str = data.substr(0, first_pipe);

  // extract timestamp
  size_t second_pipe = data.find("|", first_pipe + 1);
  std::string timestamp_str = data.substr(first_pipe + 1, second_pipe - first_pipe - 1);

  // extract
  std::string content_str = data.substr(second_pipe + 1);

  // convert timestamp back
  long long timestamp_value = std::stoll(timestamp_str);
  auto ms_since_epoch = std::chrono::milliseconds(timestamp_value);
  auto timepoint = std::chrono::system_clock::time_point(ms_since_epoch);

  // build message
  Message msg;
  msg.sender = sender_str;
  msg.timestamp = timepoint;
  msg.content = content_str;

  return msg;
}

// format time to HH:MM:SS
std::string Message::getFormattedTime() const {
  auto time_t_value = std::chrono::system_clock::to_time_t(timestamp);
  std::tm* local_time = std::localtime(&time_t_value);
  char buffer[9];
  std::strftime(buffer, sizeof(buffer), "%H:%M:%S", local_time);
  return std::string(buffer);
}
