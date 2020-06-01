#pragma once
#include <boost/asio.hpp>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <royale.hpp>

#include "udp.hpp"
#include "camera.hpp"
#include "timelog.hpp"

// camera status
struct RoyalStatus {
  bool isConnected = false;
  bool isCapturing = false;
  bool isCalibrated = false;
  bool isCalibRunning = false;
  int libraryCrashCounter = 0;
};

// potentiometer
struct PotiStatus {
  int value=0;
  bool available=false;
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

extern int imgSize;
extern bool sendImg;

extern timelog newDataLog;


// Data and their Mutexes
extern cv::Mat depImg;  // full depth image (one byte p. pixel)
extern std::mutex depImgMutex;
extern royale::DepthData dataCopy;  // storage of last depthFrame from libroyal
extern std::mutex dataCopyMutex;

// Notifying processing data (pd) thread to end waiting
extern std::condition_variable pdCond;  // depthData condition varibale
extern std::mutex pdCondMutex;          // depthData condition varibale
extern bool pdFlag;

// Counts when there is onNewData() while the previous wasn't finished yet.
// We don't want this -> royal library gets unstable
extern int lockFailCounter;

// Notifying send values to motors (sv) thread to end waiting
extern std::condition_variable svCond;  // send Values condition varibale
extern std::mutex svCondMutex;          // send Values condition varibale
extern bool svFlag;

// STRUCTS
extern RoyalStatus royalStats;
extern PotiStatus potiStats;

}  // namespace glob
