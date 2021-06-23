#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

using std::condition_variable;
using std::mutex;
using std::queue;

template <class T> class Q {

private:
  mutex mut;
  condition_variable cond;
  queue<T> q;

public:
  Q(void) : mut(), cond(), q(){};
  ~Q(void) {}
  void push(T t) {
    std::lock_guard<mutex> lock(mut);
    q.push(t);
    cond.notify_one();
  }
  // giving this a stupid name instead of the obvious pop()
  // because cpp stdlib pop() just removes the first elem
  // and returns void. argh.
  T retrieve(int timeout_millis = 0) {
    std::unique_lock<mutex> lock(mut);
    if (q.empty()) {
      if (timeout_millis == 0) {
        cond.wait(lock);
      } else {
        cond.wait_for(lock, std::chrono::milliseconds(timeout_millis));
      }
    }

    if (q.empty()) {
      // someone grabbed the queued data before us ...
      return NULL;
    }
    T value = q.front();
    q.pop();
    return value;
  }
};