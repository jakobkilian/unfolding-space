#include "timelog.hpp"
#include "camera.hpp"
#include <string>
#include <iostream>
#include <chrono>
#include <array>

using namespace std::chrono;
using std::cout;
// timelog Class
timelog::timelog(int s) {
  size = 100;
  size = s < size ? s : size;
}

void timelog::store(std::string name) {
  pos++;
  if (pos < size) {
    t[pos] = steady_clock::now();
    n[pos] = name;
  } else {
    pos = size - 1;
    cout << "timelog ERROR: End of Array \n";
  }
}
void timelog::print(const std::string instName, const std::string incr,
                           const std::string sum) {
  cout << "\n";
  cout << "-------------------------------\n";
  cout << "Failed Locks: "
       << ": " << lockFailCounter << "\n";
  cout << "-------------------------------\n";
  cout << "TIMER: " << instName << ":\n";
  cout << "-------------------------------\n";

  std::string test = "milliseconds";
  if (incr.compare("ms") == 0) {
    for (int x = 0; x < pos; x++) {
      cout << n[x + 1] << "\t was:\t "
           << duration_cast<milliseconds>(t[x + 1] - t[x]).count() << "\tms\n";
    }
  } else if (incr.compare("us") == 0) {
    for (int x = 0; x < pos; x++) {
      cout << n[x + 1] << "\t was:\t "
           << duration_cast<microseconds>(t[x + 1] - t[x]).count() << "\tus\n";
    }
  }
  cout << "_________________\n";
  if (sum.compare("ms") == 0) {
    cout << "OVERALL dur: "
         << duration_cast<milliseconds>(t[pos] - t[0]).count() << "\tms\n";
  }
  if (sum.compare("us") == 0) {
    cout << "OVERALL dur: "
         << duration_cast<microseconds>(t[pos] - t[0]).count() << "\tus\n";
  }

  pos = -1;
}
