//TODO Draft meiner Monitoring Klasse

#include <mutex>
#include <string>

class Monitoring {
 public:         // öffentlich
  Monitoring();  // der Default-Konstruktor
  void testPrint();
  void changeTestVar(int in);

 private:  // privat
  std::mutex _mut;
  struct MonitoringData {
    int var;
  } data;
};
