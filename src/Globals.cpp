/* INFO
 * These objects are stored globally to be accesible by all threads.
 *
 * Some are organized in structs (initialized in "Globals.cpp")
 * Naming convention:
 * Atomic variables (that do not need locking) begin with: a_
 * All others need to be protected manually when edited.
 * Structs therefore have a .mut member (inherited from Base) to do so.
 *
 * Those without structs need to have a separate mutex if they need one.
 *
 * All global variables are in the Glob namespace to indicate that they might
 * need mutexing...
 *
 * There might be better ways to implement multithreading, just didn't know
 * better...
 */

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

Led Glob::led1(28, 11, 27);
Led Glob::led2(10, 29, 6);

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