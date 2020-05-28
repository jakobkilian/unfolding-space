#pragma once

#include <string>
#include <array>
#include <chrono>


class timelog {
  // new: save the steps in an array of time_points to print them at the end...
  std::array<std::chrono::steady_clock::time_point, 100> t;
  std::array<std::string, 100> n;
  int size;
  int i;
  int pos = -1;

 public:
  timelog(int);
  void store(std::string name);
  void reset();
  void print(const std::string, const std::string, const std::string);
};

extern timelog mainTimeLog;
