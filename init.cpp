/*
 * File: init.cpp
 * Author: Jakob Kilian
 * Date: 26.09.19
 * Info: Initialize Things
 * Project: unfoldingspace.jakobkilian.de
 */

//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include "init.hpp"
#include "camera.hpp"
#include "glove.hpp"
#include "poti.hpp"

//----------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------

//________________________________________________
// create the windows for the visual output und position them on screen
void createWindows() {
  cv::namedWindow("tileImg8", cv::WINDOW_NORMAL);
  cv::resizeWindow("tileImg8", 672, 513);
  cv::moveWindow("tileImg8", 700, 50);
  cv::namedWindow("depImg8", cv::WINDOW_NORMAL);
  cv::resizeWindow("depImg8", 672, 513);
  cv::moveWindow("depImg8", 5, 50);
}

//________________________________________________
// check if there is a cam
bool checkCam() {
  udpHandling();
  royale::CameraManager manager;
  royale::Vector<royale::String> camlist;
  camlist = manager.getConnectedCameraList();
  if (!camlist.empty()) {
    return true;
  }
  return false;
}
