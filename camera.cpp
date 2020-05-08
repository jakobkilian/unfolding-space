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
#include <sample_utils/EventReporter.hpp>
#include <string>  // std::string, std::to_string
#include <thread>

#include "glove.hpp"
#include "init.hpp"
#include "poti.hpp"

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
std::array<uint8_t, 9> ninePixMatrix;
int globalPotiVal;

int cycleTime;
int lastPrintCurTime;
cv::Mat depImg;
cv::Mat depImgMod;
cv::Mat tileImg;

bool newDepthImage;
bool motorsMuted = false;
bool calibRunning = false;
long lastNewData = millis();

int globalCycleTime;
int globalPauseTime;

int width;
int height;

std::mutex depMutex;
std::mutex tileMutex;

// standard size of the Windows
cv::Size_<int> myWindowSize = cv::Size(112, 85);

int millisFirstFrame;
long resetFC;
bool first = false;

//----------------------------------------------------------------------
// OTHER FUNCTIONS
//----------------------------------------------------------------------

// Toggle to undistort the picture (lense produces fisheye view)
void DepthDataListener::toggleUndistort() {
  std::lock_guard<std::mutex> lock(depMutex);
  undistortImage = !undistortImage;
}

// Shift the Hue value on a 360° circle.
int myHueChange(float oldValue, float changeValue) {
  int oldInt = static_cast<int>(oldValue);
  int changeInt = static_cast<int>(changeValue);
  return (oldInt + changeInt) % 360;
}

// How long was this step?
void printCurTime(const std::string &msg) {
  // cout << millis()-lastPrintCurTime <<  " \t" << msg << endl;
  cycleTime += millis() - lastPrintCurTime;
  lastPrintCurTime = millis();
}

// How long was this cycle?
void getCycleDur() {
  cycleTime += millis() - lastPrintCurTime;
  globalCycleTime = cycleTime;
  // cout <<  " _________________________ Cycle Time:" << cycleTime <<  "
  // ______________________________" << endl;
  cycleTime = 0;
  lastPrintCurTime = millis();
}

// How long was the pause since last onNewData?
void getPauseDur() {
  cycleTime += millis() - lastPrintCurTime;
  globalPauseTime = cycleTime;
  // cout <<  "Pause Time: " << cycleTime <<  "" << endl;
  cycleTime = 0;
  lastPrintCurTime = millis();
}

// StoreTimePoint Class
storeTimePoint::storeTimePoint(int s) {
  size = 100;
  size = s < size ? s : size;
}

void storeTimePoint::store(std::string name) {
  pos++;
  if (pos < size) {
    t[pos] = steady_clock::now();
    n[pos] = name;
  } else {
    pos = size - 1;
    cout << "storeTimePoint ERROR: End of Array \n";
  }
}
void storeTimePoint::print() {
  for (int x = 0; x < pos; x++) {
    cout << n[x + 1] << "\t was:\t "
         << duration_cast<microseconds>(t[x + 1] - t[x]).count() << "\tus\n";
  }
  cout << "OVERALL duration was: "
       << duration_cast<milliseconds>(t[pos] - t[0]).count() << "\tms\n";
  pos = -1;
}

storeTimePoint camTP(20);

std::condition_variable ddCond;  // depthData condition varibale
std::mutex ddMut;                // depthData condition varibale
bool newDD = false;

// void storeTime(int i) { t[i] = steady_clock::now }

//----------------------------------------------------------------------
// MAIN FUNCTION
//----------------------------------------------------------------------

// gets called everytime there is a new depth frame from the Pico Flexx
void DepthDataListener::onNewData(const DepthData *data) {
  camTP.store("start");
  {
    std::lock_guard<std::mutex> ddLock(ddMut);
    camTP.store("lock");
    sharedData = *data;
    camTP.store("copy");
    newDD = true;
  }
  camTP.store("unlock");

  // wake other thread
  ddCond.notify_one();
  camTP.store("notif");
}

void DepthDataListener::copyData(DepthData *data) {
  camTP.store("funct");

  int histo[9][256];       // historgram, needed to find closest obj
  lastNewData = millis();  // timestamp when new frame arrives
                           // (to check if a crash of libroyale occured)
  getPauseDur();           // calculate timespan since last new "onNewData"
  if (potiAv) {
    maxDepth =
        globalPotiVal / 100 / 3;  // calculate current maxDepth from potiVal
  }

  // timestamp of the arrival of the very first frame
  if (first != true) {
    millisFirstFrame = millis();
    resetFC = millis();
    first = true;
  }

  // check dimensions of incoming data
  width = data->width;              // get width from depth image
  height = data->height;            // get height from depth image
  int tileWidth = width / 3 + 1;    // respectiveley width of one tile
  int tileHeight = height / 3 + 1;  // respectiveley height of one tile
  // this if-block is needed for mutex
  if (true) {
    std::lock_guard<std::mutex> lock(depMutex);
    depImg.create(cv::Size(width, height), CV_8UC3);   // gets filled later
    if (gui) tileImg.create(cv::Size(3, 3), CV_8UC1);  // gets filled later
    bzero(histo, sizeof(int) * 9 * 256);               // clear histogram array
    camTP.store("bf");

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
          // Depth Value in relation to maxDepth (max depth range)
          float depth = adjustDepthValue(curPoint.z, maxDepth);
          // HERE the costly stuff begins: copiing values in histogram and
          // DepImg
          // save this pixel's depth value in histo
          histo[tileIdx][(uint8_t)depth]++;
          //if pixel is in range put the right grey tone
          if (depth < 255) {
            // create a Hue value from 0-180 for the visual output
            depImgPtr[x * 3] = adjustDepthValueForImage(curPoint.z, maxDepth);
            // Saturation is always the same
            depImgPtr[x * 3 + 1] = 0;
            // create brightness value relative to depth value
            depImgPtr[x * 3 + 2] = depth;
          }
          // if pixel is out of –> make it white / invisible
          else {
            depImgPtr[x * 3] = 255;
            depImgPtr[x * 3 + 1] = 0;
            depImgPtr[x * 3 + 2] = 255;
          }
        }
        // If pixel is not valid –> make it gray
        else {
          depImgPtr[x * 3] = 7;
          depImgPtr[x * 3 + 1] = 10;
          depImgPtr[x * 3 + 2] = 245;
          histo[tileIdx][255]++;  // treat unvalid pixel as 255 = "out of range"
        }
      }
    }
  }
  newDepthImage = true;  // New Depth Image is ready
  camTP.store("aft for");

  // this if-block is needed for mutex
  if (true) {
    std::lock_guard<std::mutex> lock(tileMutex);

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
      ninePixMatrix[(tileIdx - 8) * -1] =
          (val - 255) * -1;  // Here two modification have to be done to have
                             // the right visual orientation (flip, turn)
    }

    // this if-block is needed for mutex
    if (gui) {
      if (true) {
        std::lock_guard<std::mutex> lock(depMutex);
        tileImg = cv::Mat(3, 3, CV_8UC1,
                          &ninePixMatrix);  // Write the matrix into the tileImg
      }
    }
  }
  camTP.store("aft his");

  frameCounter++;  // counting every frame

  // WRITE VALUES TO GLOVE
  // this if-block is needed for mutex
  if (true) {
    std::lock_guard<std::mutex> lock(tileMutex);
    if (motorsMuted != true &&
        calibRunning != true)  // Check if values should be written
      writeValues( 9, &(ninePixMatrix[0]));
      //std::thread{writeValues, 9, &(ninePixMatrix[0])}.detach();
    // writeValues(9, &(ninePixMatrix[0]));
  }

  // WRITE
  getCycleDur();  // Calculate how long this cycle took
  camTP.store("end");
  camTP.print();
}

float DepthDataListener::adjustDepthValue(float zValue, float max) {
  // if max is 0 (e.g. poti is at 0): set all tiles to "out of range" / 255
  if (max == 0) {
    return 255;
  }
  // if max is smaller: set all tiles to max
  if (zValue > max) {
    zValue = max;
  }
  // make ZValue relative to max and return 0-255
  float newZValue = zValue / max * 255.0f;
  return newZValue;
}

// create a Hue value from 0-180 for the visual output
float DepthDataListener::adjustDepthValueForImage(float zValue, float max) {
  if (zValue > max) {
    zValue = max;
  }
  float newZValue = zValue / max * 180.0f;
  newZValue = (newZValue - 180) * -1;
  newZValue = myHueChange(newZValue, -50);
  return newZValue;
}

// print Output to the Terminal Window for debugging and monitoring
void printOutput() {
  // print the values if the tiles as matrix
  // this if-block is needed for mutex
  if (true) {
    std::lock_guard<std::mutex> lock(tileMutex);  // lock image while reading
    for (int y = 0; y < 3; y++) {
      printf("\t");
      for (int x = 0; x < 3; x++) {
        int asciiVal = ninePixMatrix[y * 3 + x];
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
  std::lock_guard<std::mutex> lock(depMutex);
  depImgMod.create(cv::Size(width, height), CV_8UC3);
  depImg.copyTo(depImgMod);
  return depImgMod;
}

cv::Mat passUdpFrame(int incSize) {
  std::lock_guard<std::mutex> lock(depMutex);
  // constrain value between 1 and 9
  if (incSize <= 0 || incSize > 9) {
    incSize = 1;
  }
  int picSize = incSize * 20;
  cv::Size size(picSize, picSize);
  depImgMod.create(size, CV_8UC3);
  if (depImg.rows != 0) {
    cv::resize(depImg, depImgMod, size);  // resize image
  }
  return depImgMod;
}
