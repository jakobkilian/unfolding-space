#include "Globals.hpp"

#include <boost/asio.hpp>
#include <mutex>

#include "TimeLogger.hpp"
boost::asio::io_service Glob::udpService;
std::mutex Glob::udpServMux;
UdpServer Glob::udpServer(udpService, 3);

std::mutex Glob::motorBoardMux;
MotorBoard Glob::motorBoard;

std::mutex Glob::i2cMux;
I2C Glob::i2c;

std::mutex Glob::imuMux;
Imu Glob::imu;

std::atomic<int> Glob::a_lockFailCounter{0};
std::atomic<bool> Glob::a_restartUnfoldingFlag{false};
// Init structs
RoyalStatus Glob::royalStats;
Modes Glob::modes;
Motors Glob::motors;
Logger Glob::logger;
RoyalDepthData Glob::royalDepthData;
CvDepthImg Glob::cvDepthImg;
ThreadNotification Glob::notifyProcess;
ThreadNotification Glob::notifySend;
Counters Glob::counters;

//________________________________________________
// Helper function to just print a register value (int) as binary
void Glob::printBinary(uint8_t a, bool lineBreak) {
  uint8_t i;
  char breakChar;
  lineBreak ? breakChar = '\n' : breakChar = ' ';
  for (i = 0x80; i != 0; i >>= 1)
    printf("%c", (a & i) ? '1' : '0');
  printf("%c", breakChar);
}