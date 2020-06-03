#include "Globals.hpp"

#include <boost/asio.hpp>
#include <mutex>

#include "TimeLogger.hpp"
boost::asio::io_service Glob::udpService;
std::mutex Glob::udpServMux;
UdpServer Glob::udpServer(udpService, 3);

std::mutex Glob::motorBoardMux;
MotorBoard Glob::motorBoard;

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