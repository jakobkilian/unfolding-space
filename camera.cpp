//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include "camera.hpp"

#include <array>
#include <chrono>
#include <iostream>
#include <mutex>
#include <royale/IEvent.hpp>
#include <string>  // std::string, std::to_string
#include <thread>

#include "globals.hpp"
#include "glove.hpp"
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

long lastNewData = millis();

//
/******************************************************************************
 *                                 ON NEW DATA
 *                               ***************
 * gets called everytime there is a new depth frame from the Pico Flexx
 * As this is a callback function and the code is unknown we want it to
 * return as fast as possible. Therefore it only copies the data and
 * returns immidiately when the preceding frame is not yet copied.
 ******************************************************************************/
void DepthDataListener::onNewData(const DepthData *data) {
  glob::newDataLog.reset();
  glob::newDataLog.store("onNewData");
  mainTimeLog.store("endPause");
  mainTimeLog.printAll("pause", "us", "ms");
  mainTimeLog.udpTimeSpan("pause", "us", "startPause", "endPause");
  mainTimeLog.reset();
  mainTimeLog.store("start");
  mainTimeLog.store("startOnNew");
  // lock needs >1 mutexes to lock. glob::pdCondMutex therefore has no
  // meaning.
  int multiLock = try_lock(glob::dataCopyMutex, glob::pdCondMutex);
  // when lock was successfull
  if (multiLock == -1) {
    mainTimeLog.store("tryLocks");
    // simply copy data to shared memory
    glob::dataCopy = *data;
    mainTimeLog.store("copy");
    // set variable for predicate check in other thread
    glob::pdFlag = true;
    glob::dataCopyMutex.unlock();
    glob::pdCondMutex.unlock();
  } else {
    // if onNewData fails to unlock the mutex it returns instantly
    glob::lockFailCounter++;
  }
  mainTimeLog.store("unlock");
  // wake other thread
  glob::pdCond.notify_one();
  mainTimeLog.store("notifyProcessing");
}
//                                    _____
//                                 [onNewData]
//____________________________________________________________________________

/******************************************************************************
 *                                PROCESS DATA
 *                               ***************
 * Create depth Image (glob::depImg) and calculate the 9 tiles of it from which
 *the 9 vibration motors get their vibration strength value (glob::tiles)
 ******************************************************************************/
void DepthDataUtilities::processData() {
  mainTimeLog.store("startProcessing");

  int histo[9][256];  // historgram, needed to find closest obj
  // Lock Mutex for copied Data and the glob::depImg
  {
    std::lock_guard<std::mutex> dcDataLock(glob::dataCopyMutex);
    std::lock_guard<std::mutex> depDataLock(glob::depImgMutex);
    royale::DepthData *data =
        &glob::dataCopy;     // set a pointer to the copied data
    lastNewData = millis();  // timestamp when new frame arrives
    if (glob::potiStats.available) {
      maxDepth = glob::potiStats.value / 100 /
                 3;  // calculate current maxDepth from potiVal
    }

    // check dimensions of incoming data
    int width = data->width;          // get width from depth image
    int height = data->height;        // get height from depth image
    int tileWidth = width / 3 + 1;    // respectiveley width of one tile
    int tileHeight = height / 3 + 1;  // respectiveley height of one tile
    glob::depImg.create(cv::Size(width, height), CV_8UC1);  // gets filled later
    bzero(histo, sizeof(int) * 9 * 256);  // clear histogram array
    mainTimeLog.store("bf");

    // READING DEPTH IMAGE pixel by pixel
    for (int y = 0; y < height; y++) {
      unsigned char *depImgPtr = glob::depImg.ptr<uchar>(y);
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
          unsigned char depth =
              maxDepth > 0
                  ? (curPoint.z <= maxDepth
                         ? (unsigned char)(curPoint.z / maxDepth * 255.0f)
                         : 255)
                  : 255;
          // HERE the costly stuff begins: copiing values in histogram and
          // save this pixel's depth value in histo
          histo[tileIdx][depth]++;
          // if pixel is in range put the right grey tone
          if (depth < 255) {
            // create a Hue value from 0-180 for the visual output
            depImgPtr[x] = depth;
          }
          // if pixel is out of –> make it white / invisible
          else {
            depImgPtr[x] = 255;
          }
        }
        // If pixel is not valid –> make it gray
        else {
          depImgPtr[x] = 230;
          histo[tileIdx][255]++;  // treat unvalid pixel as 255 = "out of range"
        }
      }
    }
  }
  mainTimeLog.store("aft for");

  // scope is needed for glob::m_tiles
  {
    std::lock_guard<std::mutex> lock(glob::m_tiles);
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
      glob::tiles[(tileIdx - 8) * -1] = tileVal;
    }
  }
  mainTimeLog.store("aft his");
  frameCounter++;  // counting every frame

  glob::udpSendServer.preparePacket("frameCounter", frameCounter);
  // call sending thread
  {
    std::lock_guard<std::mutex> svCondLock(glob::svCondMutex);
    // mainTimeLog.store("lock");
    // mainTimeLog.store("copy");
    glob::svFlag = true;
  }
  // mainTimeLog.store("unlock");
  // wake other thread
  glob::svCond.notify_one();

  // WRITE
  mainTimeLog.store("endProcess");
  // mainTimeLog.printAll("Receiving Frame", "us", "ms");
  // mainTimeLog.reset();
}
//                                    _____
//                                [process data]
//____________________________________________________________________________

/******************************************************************************
 *                                   OTHER
 ******************************************************************************/

void EventReporter::onEvent(std::unique_ptr<royale::IEvent> &&event) {
  royale::EventSeverity severity = event->severity();
  switch (severity) {
    case royale::EventSeverity::ROYALE_INFO:
      // cerr << "info: " << event->describe() << endl;
      extractDrops(event->describe());
      break;
    case royale::EventSeverity::ROYALE_WARNING:
      // cerr << "warning: " << event->describe() << endl;
      extractDrops(event->describe());
      break;
    case royale::EventSeverity::ROYALE_ERROR:
      cerr << "error: " << event->describe() << endl;
      break;
    case royale::EventSeverity::ROYALE_FATAL:
      cerr << "fatal: " << event->describe() << endl;
      break;
    default:
      // cerr << "waits..." << event->describe() << endl;
      break;
  }
}
//________________________________________________
// Royale Event Listener reports dropped frames as string.
// This functions extracts the number of frames that got lost at Bridge/FC.
// I believe, that dropped frames cause instability – PMDtec confirmed this
void EventReporter::extractDrops(royale::String str) {
  using namespace std;
  stringstream ss;
  /* Storing the whole string into string stream */
  ss << str;
  /* Running loop till the end of the stream */
  string temp;
  int found;
  int i = 0;
  while (!ss.eof()) {
    /* extracting word by word from stream */
    ss >> temp;
    /* Checking the given word is integer or not */
    if (stringstream(temp) >> found) {
      if (i == 0) glob::udpSendServer.preparePacket("drpBridge", found);
      if (i == 1) glob::udpSendServer.preparePacket("drpFC", found);
      if (i == 2) glob::udpSendServer.preparePacket("delivFrames", found);
      i++;
    }
    /* To save from space at the end of string */
    temp = "";
  }
  // glob::udpSendServer.preparePacket("11", tenSecsDrops);
  // tenSecsDrops += droppedAtBridge + droppedAtFC;
}

cv::Mat DepthDataUtilities::getResizedDepthImage(int incSize) {
  std::lock_guard<std::mutex> lock(glob::depImgMutex);
  cv::Mat sizedImgCopy;
  // constrain value between 1 and 9
  if (incSize <= 0 || incSize > 9) {
    incSize = 1;
  }
  int picSize = incSize * 20;
  cv::Size size(picSize, picSize);
  sizedImgCopy.create(size, CV_8UC1);
  if (glob::depImg.rows != 0) {
    cv::resize(glob::depImg, sizedImgCopy, size);  // resize image
  }
  return sizedImgCopy;
}
