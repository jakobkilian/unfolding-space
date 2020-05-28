/*
 * File: camera.cpp
 * Author: Jakob Kilian
 * Date: 26.09.19
 * Info: Handles incoming frames of the pico flexx +  image processing
 * Project: unfoldingspace.jakobkilian.de
 */

//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include "camera.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>  // std::string, std::to_string
#include <thread>

#include "glove.hpp"
#include "init.hpp"
#include "poti.hpp"
#include "timelog.hpp"

using std::cerr;
using std::cout;
using std::endl;
using namespace royale;
using namespace std::chrono;

//----------------------------------------------------------------------
// DECLARATIONS AND VARIABLES
//----------------------------------------------------------------------
int minObjSizeThresh = 90;  // the min number of pixels, an object must have
                            // (smaller objects might be noise)
float maxDepth = 1.5;       // The depth of viewing range.
                            // Objects with bigger distance to camera
                            // are ignored. Value can be changed by poti.

int frameCounter;  // counter for single frames
float fps;         // av. frames per second
int globalPotiVal;
int lockFailCounter = 0;

int cycleTime;
int lastPrintCurTime;
cv::Mat depImgMod;
cv::Mat tileImg;

bool motorsMuted = false;
bool calibRunning = false;
long lastNewData = millis();

int width;  // needed by passdepim zeug TODO
int height;

// Data and their Mutexes
cv::Mat depImg;  // full depth image (one byte p. pixel)
std::mutex depImgMutex;
int tilesArray[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};  // 9 tiles | motor vals
std::mutex tilesMutex;
 std::mutex motorTestMutex;
royale::DepthData dataCopy;  // storage of last depthFrame from libroyal
std::mutex dataCopyMutex;

// Notifying processing data (pd) thread to end waiting
std::condition_variable pdCond;  // depthData condition varibale
std::mutex pdCondMutex;          // depthData condition varibale
bool pdFlag = false;

// Notifying send values to motors (sv) thread to end waiting
std::condition_variable svCond;  // send Values condition varibale
std::mutex svCondMutex;          // send Values condition varibale
bool svFlag = false;

//----------------------------------------------------------------------
// MAIN FUNCTION
//----------------------------------------------------------------------

// gets called everytime there is a new depth frame from the Pico Flexx
// As this is a callback function and the code is unknown we want it to
// return as fast as possible. Therefore it only copies the data and
// returns immidiately when the preceding frame is not yet copied.
void DepthDataListener::onNewData(const DepthData *data) {
  mainTimeLog.store("start");
  // lock needs >1 mutexes to lock. pdCondMutex therefore has no
  // meaning.
  int multiLock = try_lock(dataCopyMutex, pdCondMutex);
  // when lock was successfull
  if (multiLock == -1) {
    mainTimeLog.store("lock");
    // simply copy data to shared memory
    dataCopy = *data;
    mainTimeLog.store("copy");
    // set variable for predicate check in other thread
    pdFlag = true;
    dataCopyMutex.unlock();
    pdCondMutex.unlock();
  } else {
    // if onNewData fails to unlock the mutex it returns instantly
    lockFailCounter++;
  }
  mainTimeLog.store("unlock");
  // wake other thread
  pdCond.notify_one();
  mainTimeLog.store("notif");
}

// Process data
// Create depth Image (depImg) and calculate the 9 tiles of it from which the 9
// vibration motors get their vibration strength value (tilesArray)
void DepthDataListener::processData() {
  mainTimeLog.store("funct");
  int histo[9][256];  // historgram, needed to find closest obj
  // Lock Mutex for copied Data and the depImg
  {
    std::lock_guard<std::mutex> dcDataLock(dataCopyMutex);
    std::lock_guard<std::mutex> depDataLock(depImgMutex);
    DepthData *data = &dataCopy;  // set a pointer to the copied data
    lastNewData = millis();       // timestamp when new frame arrives
    if (potiAv) {
      maxDepth =
          globalPotiVal / 100 / 3;  // calculate current maxDepth from potiVal
    }

    // check dimensions of incoming data
    width = data->width;              // get width from depth image
    height = data->height;            // get height from depth image
    int tileWidth = width / 3 + 1;    // respectiveley width of one tile
    int tileHeight = height / 3 + 1;  // respectiveley height of one tile
    depImg.create(cv::Size(width, height), CV_8UC1);  // gets filled later
    bzero(histo, sizeof(int) * 9 * 256);              // clear histogram array
    mainTimeLog.store("bf");

    // READING DEPTH IMAGE pixel by pixel
    for (int y = 0; y < height; y++) {
      uint8_t *depImgPtr = depImg.ptr<uint8_t>(y);
      for (int x = 0; x < width; x++) {
        // save currently observed pixel in curPoint
        auto curPoint = data->points.at(y * width + x);
        // check its validity (if bigger than 10 -> valid)
        bool valid = curPoint.depthConfidence > 10;
        // select the respective tile
        int tileIdx = (x / tileWidth) + 3 * (y / tileHeight);

        // WRITE VALID PIXELS in DepImg and histogram
        if (valid) {
          // if maxDepth is set to 0 by poti -> out of range (255)
          // if value exceeds maxDepth -> out of range (255)
          // else -> calc value between 0 to 255
          uint8_t depth = maxDepth > 0
                              ? (curPoint.z <= maxDepth
                                     ? (uint8_t)(curPoint.z / maxDepth * 255.0f)
                                     : 255)
                              : 255;
          // HERE the costly stuff begins: copiing values in histogram and
          // save this pixel's depth value in histo
          histo[tileIdx][depth]++;
          // if pixel is in range put the right grey tone
          if (depth < 255) {
            // create a Hue value from 0-180 for the visual output
            depImgPtr[x * 3] = depth;
            // Saturation is always the same
            // depImgPtr[x * 3 + 1] = 0;
            // create brightness value relative to depth value
            // depImgPtr[x * 3 + 2] = depth;
          }
          // if pixel is out of –> make it white / invisible
          else {
            depImgPtr[x * 3] = 255;
            // depImgPtr[x * 3 + 1] = 0;
            // depImgPtr[x * 3 + 2] = 255;
          }
        }
        // If pixel is not valid –> make it gray
        else {
          depImgPtr[x * 3] = 230;
          // depImgPtr[x * 3 + 1] = 10;
          // depImgPtr[x * 3 + 2] = 245;
          histo[tileIdx][255]++;  // treat unvalid pixel as 255 = "out of range"
        }
      }
    }
  }
  mainTimeLog.store("aft for");

  // scope is needed for tilesMutex
  {
    std::lock_guard<std::mutex> lock(tilesMutex);
    // FIND CLOSEST object in each tile
    for (int tileIdx = 0; tileIdx < 9; tileIdx++) {
      int sum = 0;
      int val;
      int offset =
          17;          // exclude the first 17cm because of oversaturation
                       // issues and noisy data the Pico Flexx has in this range
      int range = 50;  // look in a tolerance range of 50cm
      for (val = offset; val < 256; val++) {
        if (histo[tileIdx][val] > 5) {
          sum += histo[tileIdx][val];
        }
        if (val > range + offset) {
          if (histo[tileIdx][val - range] > 5) {
            sum -= histo[tileIdx][val - range];
          }
        }
        if (sum >= minObjSizeThresh)  // if minObjSizeThresh is exeeded: break.
                                      // val now holds the depth for this tile
          break;
      }
      // WRITE the value in the Tile Matrix:
      // Here two modification have to be done to have
      // the right visual orientation (flip, turn)
      int tileVal = (val - 255) * -1;
      // TODO: why can this happen?
      if (tileVal < 0) {
        tileVal = 0;
        std::cout << "there was a -1\n";
      }
      tilesArray[(tileIdx - 8) * -1] = tileVal;
    }
  }
  mainTimeLog.store("aft his");
  frameCounter++;  // counting every frame

  // call sending thread
  {
    std::lock_guard<std::mutex> svCondLock(svCondMutex);
    // mainTimeLog.store("lock");
    // mainTimeLog.store("copy");
    svFlag = true;
  }
  // mainTimeLog.store("unlock");
  // wake other thread
  svCond.notify_one();

  // queue the sending task in the udp server TODO
  //  w->udpSendServer->postSend();

  // WRITE
  mainTimeLog.store("pro end");
  // mainTimeLog.print("Receiving Frame", "us", "ms");
}

// print Output to the Terminal Window for debugging and monitoring
void printOutput() {
  // print the values if the tiles as matrix
  // this if-block is needed for mutex
  if (true) {
    std::lock_guard<std::mutex> lock(tilesMutex);  // lock image while reading
    for (int y = 0; y < 3; y++) {
      printf("\t");
      for (int x = 0; x < 3; x++) {
        int asciiVal = tilesArray[y * 3 + x];
        if (asciiVal > 250) {
          printf("\t-\t");
        } else {
          printf("\t%i\t", asciiVal);
        }
      }
      printf("\n\n\n");
    }
    printf("\n\n\n");
  }
}

// This function can be called by main.cpp to get the depth values of the nine
// tiles
cv::Mat passNineFrame() { return tileImg; }

// This function can be called by main.cpp to get the detailed depth image
cv::Mat passDepFrame() {
  std::lock_guard<std::mutex> lock(depImgMutex);
  depImgMod.create(cv::Size(width, height), CV_8UC3);
  // cv::cvtColor(depImg, depImgMod, CV_GRAY2RGB);

  // depImg.copyTo(depImgMod);
  return depImgMod;
}

cv::Mat passUdpFrame(int incSize) {
  std::lock_guard<std::mutex> lock(depImgMutex);
  // constrain value between 1 and 9
  if (incSize <= 0 || incSize > 9) {
    incSize = 1;
  }
  int picSize = incSize * 20;
  cv::Size size(picSize, picSize);
  depImgMod.create(size, CV_8UC1);
  if (depImg.rows != 0) {
    cv::resize(depImg, depImgMod, size);  // resize image
  }
  return depImgMod;
}
