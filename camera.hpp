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
#include <mutex>
#include <opencv2/opencv.hpp>
#include <royale.hpp>
#include <string.h>

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

//----------------------------------------------------------------------
// CLASSES
//----------------------------------------------------------------------
class DepthDataListener : public royale::IDepthDataListener
{
  // lens matrices used for the undistortion of
  // the image
  cv::Mat cameraMatrix;
  cv::Mat distortionCoefficients;
  bool undistortImage = false;

public:
  DepthDataListener() : undistortImage(false) {}
  void onNewData(const royale::DepthData *data);
  void setLensParameters(const royale::LensParameters &lensParameters);
  void toggleUndistort();

private:
  float adjustDepthValue(float zValue, float max);
  float adjustDepthValueForImage(float zValue, float max);
};
