#include "globals.hpp"

#include <boost/asio.hpp>
#include <mutex>

#include "timelog.hpp"
boost::asio::io_service glob::udpService;
std::mutex glob::udpServMux;
udp_server glob::udpServer(udpService, 3);
std::atomic<int> glob::a_lockFailCounter{0};
std::atomic<bool> glob::a_restartUnfoldingFlag{false};
// Init structs
RoyalStatus glob::royalStats;
PotiStatus glob::potiStats;
Modes glob::modes;
Motors glob::motors;
Logger glob::logger;
RoyalDepthData glob::royalDepthData;
CvDepthImg glob::cvDepthImg;
ThreadNotification glob::notifyProcess;
ThreadNotification glob::notifySend;