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
#include <royale/IEvent.hpp>
#include "globals.hpp"


#include "timelog.hpp"

using namespace std::chrono;

//----------------------------------------------------------------------
// DECLARATIONS
//----------------------------------------------------------------------
cv::Mat passUdpFrame(int);
void printOutput();

// TODO: global variables from main.cpp
extern long timeSinceLastNewData;  // time passed since last "onNewData"
extern int fpsFromCam;       // wich royal use case is used? (how many fps?)
extern int currentKey;       //
//extern int libraryCrashNo;   // counter for the crashes of the library
extern int
    longestTimeNoData;  // the longest timespan without new data since start
extern bool cameraDetached;   // camera got detached
extern long cameraStartTime;  // timestamp when camera started capturing
extern bool record;           // currently recording?
extern long lastNewData;
extern int frameCounter;
extern int kCounter;

// Data and their Mutexes
extern cv::Mat depImg;  // full depth image (one byte p. pixel)
extern std::mutex depImgMutex;
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
//   boost::asio::io_service udpSendService;
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



//________________________________________________
// Gets called by Royale irregularily.
// Holds the camera state, errors and info about drops
class EventReporter : public royale::IEventListener {
 public:
  virtual ~EventReporter() = default;

virtual void onEvent(std::unique_ptr<royale::IEvent> &&event) override;
private:
void extractDrops(royale::String str);

  };