#include "timelog.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <string>

#include "camera.hpp"
#include "globals.hpp"

using namespace std::chrono;
using std::cout;

void timelog::store(const std::string name) {
  std::lock_guard<std::mutex> lock(mut);
  timePoint.push_back(steady_clock::now());
  nameTag.push_back(name);
}

void timelog::printAll(const std::string instName, const std::string incr,
                       const std::string sum) {
                          std::lock_guard<std::mutex> lock(mut);
  if (timePoint.size() > 1) {  // if there is sth to print
    cout << "\n";
    cout << "-------------------------------\n";
    cout << "TIMER: " << instName << ":\n";
    cout << "-------------------------------\n";
    if (incr.compare("ms") == 0) {
      for (int x = 1; x < timePoint.size(); x++) {
        cout << duration_cast<milliseconds>(timePoint[x] - timePoint[x - 1])
                    .count()
             << "\tms"
             << "\t from: \t " << nameTag[x] << "\n";
      }
    } else if (incr.compare("us") == 0) {
      for (int x = 1; x < timePoint.size(); x++) {
        cout << duration_cast<microseconds>(timePoint[x] - timePoint[x - 1])
                    .count()
             << "\tus"
             << "\t from: \t " << nameTag[x] << "\n";
      }
    }
    cout << "_________________\n";
    if (sum.compare("ms") == 0) {
      cout << "OVERALL dur: "
           << duration_cast<milliseconds>(timePoint[timePoint.size() - 1] -
                                          timePoint[0])
                  .count()
           << "\tms\n";
    }
    if (sum.compare("us") == 0) {
      cout << "OVERALL dur: "
           << duration_cast<microseconds>(timePoint[timePoint.size() - 1] -
                                          timePoint[0])
                  .count()
           << "\tus\n";
    }

  } else {
    cout << "-------------------------------\n";
    cout << "Error: Nothing to print! (less than 2 values stored)\n";
    cout << "-------------------------------\n";
  }
}

void timelog::udpTimeSpan(std::string ident, std::string incr, std::string from,
                          std::string to) {
                             std::lock_guard<std::mutex> lock(mut);
  // find "from" keyword and "to" keyword and return iterator
  std::vector<std::string>::iterator fromIt =
      std::find(nameTag.begin(), nameTag.end(), from);
  std::vector<std::string>::iterator toIt =
      std::find(nameTag.begin(), nameTag.end(), to);
  if (fromIt != nameTag.end() && toIt != nameTag.end()) {  // when in bound:
    int fromInd = std::distance(nameTag.begin(), fromIt);  // get indexes
    int toInd = std::distance(nameTag.begin(), toIt);      // get indexes
    unsigned int duration = 0;
    if (incr.compare("ms") == 0) {
      duration =
          duration_cast<milliseconds>(timePoint[toInd] - timePoint[fromInd])
              .count();
    } else if (incr.compare("us") == 0) {
      duration =
          duration_cast<microseconds>(timePoint[toInd] - timePoint[fromInd])
              .count();
    }
    glob::udpServer.preparePacket(ident, duration);
  }
}

void timelog::reset() {
   std::lock_guard<std::mutex> lock(mut);
  timePoint.clear();
  nameTag.clear();
}

long timelog::msSinceEntry(int id) {
   std::lock_guard<std::mutex> lock(mut);
  long val = 0;
  if (timePoint.size() > id) {  // if there is a first entry
    val = duration_cast<milliseconds>(steady_clock::now() - timePoint[id])
              .count();
  } else {
    val = -1;
  }
  return val;
}
