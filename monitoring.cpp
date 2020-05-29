//TODO Draft meiner Monitoring Klasse

#include "monitoring.hpp"

#include <iostream>
#include <mutex>
#include <string>

Monitoring::Monitoring() {}

void Monitoring::testPrint() {
  std::lock_guard<std::mutex> lock(_mut);
  std::cout << "Test Var is: " << data.var  << "\n";
}

void Monitoring::changeTestVar(int in) {
  std::lock_guard<std::mutex> lock(_mut);
  data.var = in;
}