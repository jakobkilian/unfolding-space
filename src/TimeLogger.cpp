#include "TimeLogger.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <string>

#include "Camera.hpp"
#include "Globals.hpp"

using namespace std::chrono;
using std::cout;

void TimeLogger::store(const std::string name) {
  if (Glob::modes.a_doLog == true) {
    std::lock_guard<std::mutex> lockStore(mut);
    timePoint.push_back(steady_clock::now());
    nameTag.push_back(name);
  }
}

void TimeLogger::printAll(const std::string instName, const std::string incr,
                          const std::string sum) {
  if (Glob::modes.a_doLogPrint == true && Glob::modes.a_doLog == true) {
    std::lock_guard<std::mutex> lockPrintAll(mut);
    if (timePoint.size() > 1) { // if there is sth to print
      cout << "\n";
      cout << "-------------------------------\n";
      cout << "TIMER: " << instName << ":\n";
      cout << "-------------------------------\n";

      for (unsigned int x = 1; x < timePoint.size(); x++) {
        auto timeSpan = timePoint[x] - timePoint[x - 1];
        if (incr.compare("ms") == 0) {
          cout << duration_cast<milliseconds>(timeSpan).count();
        } else if (incr.compare("us") == 0) {
          cout << duration_cast<microseconds>(timeSpan).count();
        }
        cout << "\t" << incr << "\t from: \t " << nameTag[x] << "\n";
      }

      cout << "_________________\n";
      cout << "OVERALL dur: ";
      auto timeSum = timePoint[timePoint.size() - 1] - timePoint[0];
      if (sum.compare("ms") == 0) {
        cout << duration_cast<milliseconds>(timeSum).count();
      }
      if (sum.compare("us") == 0) {
        cout << duration_cast<microseconds>(timeSum).count();
      }
      cout << "\t" << sum << "\n";
    } else {
      cout << "-------------------------------\n";
      cout << "Error: Nothing to print! (less than 2 values stored)\n";
      cout << "-------------------------------\n";
    }
  }
}

void TimeLogger::udpTimeSpan(std::string ident, std::string incr,
                             std::string from, std::string to) {
  if (Glob::modes.a_doLog == true) {
    std::lock_guard<std::mutex> lockTimeSpan(mut);
    // find "from" keyword and "to" keyword and return iterator
    std::vector<std::string>::iterator fromIt =
        std::find(nameTag.begin(), nameTag.end(), from);
    std::vector<std::string>::iterator toIt =
        std::find(nameTag.begin(), nameTag.end(), to);
    if (fromIt != nameTag.end() && toIt != nameTag.end()) { // when in bound:
      int fromInd = std::distance(nameTag.begin(), fromIt); // get indexes
      int toInd = std::distance(nameTag.begin(), toIt);     // get indexes
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
      {
        std::lock_guard<std::mutex> lockSendDur(Glob::udpServMux);
        Glob::udpServer.preparePacket(ident, duration);
      }
    }
  }
}

void TimeLogger::reset() {
  if (Glob::modes.a_doLog == true) {
    std::lock_guard<std::mutex> lockClear(mut);
    timePoint.clear();
    nameTag.clear();
  }
}

long TimeLogger::msSinceEntry(unsigned int id) {
  std::lock_guard<std::mutex> lockGetDur(mut);
  long val = 0;
  if (timePoint.size() > id) { // if there is a first entry
    val = duration_cast<milliseconds>(steady_clock::now() - timePoint[id])
              .count();
  } else {
    val = -1;
  }
  return val;
}
