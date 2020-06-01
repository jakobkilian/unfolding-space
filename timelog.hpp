#pragma once

#include <array>
#include <chrono>
#include <string>
#include <vector>

class timelog {
  std::vector<std::chrono::steady_clock::time_point> timePoint;
  std::vector<std::string> nameTag;
  int i;
  int pos = -1;

 public:
  void store(std::string name);
  void reset();
  //void sendAll(const std::string, const std::string, const std::string);
  void printAll(const std::string, const std::string, const std::string);
  void udpTimeSpan(std::string ident,std::string incr,std::string from, std::string to);
  long msSinceEntry(int id);
};

extern timelog mainTimeLog;
