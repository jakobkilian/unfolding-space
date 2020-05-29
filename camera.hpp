/*
 * File: camera.hpp
 * Author: Jakob Kilian
 * Date: 26.09.19
 * Project: unfoldingspace.jakobkilian.de
 */

#pragma once

//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include <string.h>

#include <condition_variable>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <royale.hpp>
#include <thread>

#include "timelog.hpp"

using namespace std::chrono;

//----------------------------------------------------------------------
// DECLARATIONS
//----------------------------------------------------------------------
cv::Mat passUdpFrame(int);
void printOutput();

// TODO: global variables from main.cpp
extern long timeSinceLastNewData;  // time passed since last "onNewData"
extern double coreTempDouble;      // Temperature of the Raspi core
extern int
    droppedAtBridge;     // How many frames got dropped at Bridge (in libroyale)
extern int droppedAtFC;  // How many frames got dropped at FC (in libroyale)
extern int deliveredFrames;  // Number of frames delivered
extern int tenSecsDrops;     // Number of drops in the last 10 seconds
extern int fpsFromCam;       // wich royal use case is used? (how many fps?)
extern int currentKey;       //
extern int libraryCrashNo;   // counter for the crashes of the library
extern int
    longestTimeNoData;  // the longest timespan without new data since start
extern bool connected;  // camera is currently connected
extern bool capturing;  // camera is currently capturing
extern bool cameraDetached;   // camera got detached
extern long cameraStartTime;  // timestamp when camera started capturing
extern bool record;           // currently recording?
extern bool motorsMuted;
extern bool testMotors;
extern int motorTestMatrix[9];
extern bool calibRunning;
extern long lastNewData;
extern int frameCounter;
extern int kCounter;
extern float fps;

// Data and their Mutexes
extern cv::Mat depImg;  // full depth image (one byte p. pixel)
extern std::mutex depImgMutex;
extern int tilesArray[];  // 9 tiles | motor vals
extern std::mutex tilesMutex;
extern std::mutex motorTestMutex;
extern royale::DepthData dataCopy;  // storage of last depthFrame from libroyal
extern std::mutex dataCopyMutex;

// Notifying processing data (pd) thread to end waiting
extern std::condition_variable pdCond;  // depthData condition varibale
extern std::mutex pdCondMutex;          // depthData condition varibale
extern bool pdFlag;

// Notifying send values to motors (sv) thread to end waiting
extern std::condition_variable svCond;  // send Values condition varibale
extern std::mutex svCondMutex;          // send Values condition varibale
extern bool svFlag;

extern int lockFailCounter;

//----------------------------------------------------------------------
// CLASSES
//----------------------------------------------------------------------
class DepthDataListener : public royale::IDepthDataListener {
 public:
  void onNewData(const royale::DepthData *data);
  void processData();
  void setLensParameters(const royale::LensParameters &lensParameters);

 private:
  uint8_t adjustDepthValue(float zValue, float max);
  float adjustDepthValueForImage(float zValue, float max);
  // TODO: set globals as private...
  // long timeSinceLastNewData;  // time passed since last "onNewData"
  // int longestTimeNoData;  // the longest timespan without new data since start
  // int frameCounter;
};

extern royale::DepthData dataCopy;

// // TODO: unsauber das hier zu machen....?
// class mainThreadWrapper {
//  public:
//   udp_server *udpSendServer;
//   boost::asio::io_service udpSendService;
//   boost::asio::io_service udpBroadService;
//   void runUdpSend();
//   std::thread runUdpSendThread();
//   void runUdpRec();
//   std::thread runUdpRecThread();
//   void runUdpBroad();
//   std::thread runUdpBroadThread();
//   void runUnfolding();
//   std::thread runUnfoldingThread();
//   void runCopyDepthData();
//   std::thread runCopyDepthDataThread();
//   void runSendDepthData() ;
//   std::thread runSendDepthDataThread();
// };

// extern mainThreadWrapper *w;