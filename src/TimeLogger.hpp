#pragma once

#include <array>
#include <chrono>
#include <mutex>
#include <string>
#include <vector>

class TimeLogger {
  std::vector<std::chrono::steady_clock::time_point> timePoint;
  std::vector<std::string> nameTag;
  int i;
  int pos = -1;
  std::mutex mut;

public:
  void store(std::string name);
  void reset();
  // void sendAll(const std::string, const std::string, const std::string);
  void printAll(const std::string, const std::string, const std::string);
  void udpTimeSpan(std::string ident, std::string incr, std::string from,
                   std::string to);
  long msSinceEntry(unsigned int id);
};
