#pragma once
//TODO Draft meiner ProtectedData Klasse

#include <mutex>
#include <string>

class ProtectedData {
 private:
  // Struct to store data.
  // Core concern: How can I access this in a thread-safe manner?
  struct Data {
    int testInt;
    bool testBool;
    bool isMuted;
  };
  Data _data;         // Here my data gets stored
  std::mutex _mutex;  // private mutex to achieve protection

  // As long it is in scope this protecting wrapper keeps the mutex locked
  // and provides a public way to access the data structure
  class ProtectingWrapper {
   public:
    ProtectingWrapper(Data& data, std::mutex& mutex)
        : data(data), _lock(mutex) {}
    Data& data;
    std::unique_lock<std::mutex> _lock;
  };

 public:
  // public function to return an instance of this protecting wrapper
  ProtectingWrapper access();
};

//Instace get declared in definitions.hpp
