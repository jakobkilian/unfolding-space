#pragma once
#include <boost/asio.hpp>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <royale.hpp>

#include "Camera.hpp"
#include "MotorBoard.hpp"
#include "TimeLogger.hpp"
#include "UdpServer.hpp"

/*
Globals are organized in structs, that get initialized in "Globals.cpp"
Naming convention:
Atomic variables (that do not need locking) are beginning with: a_
All others need to be protected when edited.
Every Struct therefore has a .mut member (inherited from Base) to do so.
*/

struct Base {
  std::mutex mut;
};

// camera status
struct RoyalStatus {
  std::atomic<bool> a_isConnected{false};
  std::atomic<bool> a_isCapturing{false};
  std::atomic<bool> a_isCalibrated{false};
  std::atomic<bool> a_isCalibRunning{false};
  std::atomic<int> a_libraryCrashCounter{0};
};

struct Modes  {
  std::atomic<bool> a_muted;
  std::atomic<bool> a_testMode;
  std::atomic<unsigned int> a_cameraUseCase{3};
};

struct Motors : Base {
  std::atomic<bool> a_muted;
  std::atomic<bool> a_testMode;
  unsigned char testTiles[9];
  unsigned char tiles[9];  // 9 tiles | motor valsˇ
};

struct Logger : Base {
  TimeLogger newDataLog;
  TimeLogger mainLogger;
};

struct CvDepthImg : Base {
  cv::Mat mat;  // full depth image (one byte p. pixel)
};

struct RoyalDepthData : Base {
  royale::DepthData dat;
};

struct ThreadNotification : Base {
  std::condition_variable cond;
  bool flag{false};
};

struct Counters {
 std::atomic<long> frameCounter;
};

// Everything goes in the namespace "Glob"
namespace Glob {
//protection needed?
extern std::mutex udpServMux;
extern boost::asio::io_service udpService;
extern UdpServer udpServer;

extern std::mutex motorBoardMux;
extern MotorBoard motorBoard;

// Counts when there is onNewData() while the previous wasn't finished yet.
// We don't want this -> royal library gets unstable
extern std::atomic<int> a_lockFailCounter;
extern std::atomic<bool> a_restartUnfoldingFlag;


// INIT ALL STRUCTS
extern RoyalStatus royalStats;
extern Modes modes;
extern Motors motors;
extern Logger logger;
extern CvDepthImg cvDepthImg;
extern RoyalDepthData royalDepthData;
extern ThreadNotification notifyProcess;
extern ThreadNotification notifySend;
extern Counters counters;
}  // namespace Glob
