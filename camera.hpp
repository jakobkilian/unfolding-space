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

using namespace std::chrono;

//----------------------------------------------------------------------
// DECLARATIONS
//----------------------------------------------------------------------
void printCurTime(const std::string &);
cv::Mat passDepFrame();
cv::Mat passUdpFrame(int);
cv::Mat passNineFrame();
void printOutput();
std::string packValStr();

extern int globalCycleTime;
extern int globalPauseTime;
extern bool motorsMuted;
extern bool calibRunning;
extern bool noChangeInMatrix;
extern bool newDepthImage;
extern bool processingImg;
extern std::mutex depMutex;
extern long lastNewData;
extern int frameCounter;
extern int kCounter;
extern float fps;
extern std::array<uint8_t, 9> ninePixMatrix;
extern bool gui;
extern long resetFC;
extern std::condition_variable ddCond;  // depthData condition varibale
extern std::mutex ddMut;                // depthData condition varibale
extern bool newDD; 

//----------------------------------------------------------------------
// CLASSES
//----------------------------------------------------------------------
class DepthDataListener : public royale::IDepthDataListener {
  // lens matrices used for the undistortion of
  // the image
  cv::Mat cameraMatrix;
  cv::Mat distortionCoefficients;
  bool undistortImage = false;

 public:
  DepthDataListener() : undistortImage(false) {}
  void onNewData(const royale::DepthData *data);
  void copyData( royale::DepthData *data);
  void setLensParameters(const royale::LensParameters &lensParameters);
  void toggleUndistort();

 private:
  float adjustDepthValue(float zValue, float max);
  float adjustDepthValueForImage(float zValue, float max);
};

class storeTimePoint {
  // new: save the steps in an array of time_points to print them at the end...
  std::array<steady_clock::time_point, 100> t;
  std::array<std::string, 100> n;
  int size;
  int i;
  int pos;

 public:
  storeTimePoint(int s);
  void store(std::string name);
  void reset();
  void print();
};

extern royale::DepthData sharedData;
extern storeTimePoint camTP;