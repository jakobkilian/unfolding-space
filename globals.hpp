#pragma once
#include <boost/asio.hpp>
#include <mutex>

#include "udp.hpp"


// camera status
struct RoyalStatus {
  bool isConnected = false;
  bool isCapturing = false;
  bool isCalibrated = false;
  bool isCalibRunning =false;
  int  libraryCrashCounter =0;
};

namespace glob {
extern udp_server udpSendServer;
extern boost::asio::io_service udpSendService;
extern std::mutex m_universal;
extern bool testBool;
extern bool isMuted;
extern bool isTestMode;
extern std::mutex m_testTiles;
extern unsigned char testTiles[9];
extern std::mutex m_tiles;
extern unsigned char tiles[9];  // 9 tiles | motor vals
extern RoyalStatus royalStats;
extern int imgSize;
extern bool sendImg;
extern bool potiAvailable;
}  // namespace glob

