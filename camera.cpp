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
  glob::logger.newDataLog.reset();
  glob::logger.newDataLog.store("onNewData");
  glob::logger.mainLogger.store("endPause");
  // glob::logger.mainLogger.printAll("pause", "us", "ms");
  glob::logger.mainLogger.udpTimeSpan("pause", "us", "startPause", "endPause");
  glob::logger.mainLogger.reset();
  glob::logger.mainLogger.store("start");
  glob::logger.mainLogger.store("startOnNew");
  int multiLock = try_lock(glob::royalDepthData.mut, glob::notifyProcess.mut);
  // when lock was successfull
  if (multiLock == -1) {
    glob::logger.mainLogger.store("tryLocks");
    // simply copy data to shared memory
    glob::royalDepthData.dat = *data;
    glob::logger.mainLogger.store("copy");
    // set variable for predicate check in other thread
    glob::notifyProcess.flag = true;
    glob::royalDepthData.mut.unlock();
    glob::notifyProcess.mut.unlock();

  } else {
    // if onNewData fails to unlock the mutex it returns instantly
    glob::a_lockFailCounter++;
    std::cout << "failed locks:" << glob::a_lockFailCounter
              << " last is: " << multiLock << "\n";
    if (multiLock == 0) {
      glob::notifyProcess.mut.unlock();
    }
    if (multiLock == 1) {
      glob::royalDepthData.mut.unlock();
    }
  }

  glob::logger.mainLogger.store("unlock");
  // wake other thread
  glob::notifyProcess.cond.notify_one();
  glob::logger.mainLogger.store("notifyProcessing");
}
//                                    _____
//                                 [onNewData]
//____________________________________________________________________________

/******************************************************************************
 *                                PROCESS DATA
 *                               ***************
 * Create depth Image (glob::cvDepthImg.mat) and calculate the 9 tiles of it
 *from which the 9 vibration motors get their vibration strength value
 *(glob::motors.tiles)
 ******************************************************************************/
void DepthDataUtilities::processData() {
  glob::logger.mainLogger.store("startProcessing");
  int histo[9][256];  // historgram, needed to find closest obj
  // Lock Mutex for copied Data and the glob::cvDepthImg.mat
  {
    royale::DepthData *data;
    {
      std::lock_guard<std::mutex> depDataLock(glob::royalDepthData.mut);
      data = &glob::royalDepthData.dat;  // set a pointer to the copied data
    }
    lastNewData = millis();  // timestamp when new frame arrives
    if (glob::potiStats.a_potAvail) {
      maxDepth = glob::potiStats.a_potVal / 100 /
                 3;  // calculate current maxDepth from potiVal
    }
    // check dimensions of incoming data
    int width = data->width;          // get width from depth image
    int height = data->height;        // get height from depth image
    int tileWidth = width / 3 + 1;    // respectiveley width of one tile
    int tileHeight = height / 3 + 1;  // respectiveley height of one tile
    // scope for mutex
    {
      std::lock_guard<std::mutex> dcDataLock(glob::cvDepthImg.mut);
      glob::cvDepthImg.mat.create(cv::Size(width, height),
                                  CV_8UC1);  // gets filled later
    }

    bzero(histo, sizeof(int) * 9 * 256);  // clear histogram array
    glob::logger.mainLogger.store("bf");

    // READING DEPTH IMAGE pixel by pixel
    for (int y = 0; y < height; y++) {
      unsigned char *depImgPtr;
      {
        std::lock_guard<std::mutex> dcDataLock(glob::cvDepthImg.mut);
        depImgPtr = glob::cvDepthImg.mat.ptr<uchar>(y);
      }
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
  glob::logger.mainLogger.store("aft for");

  {
    // FIND CLOSEST object in each tile
    for (int tileIdx = 0; tileIdx < 9; tileIdx++) {
      int sum = 0;
      int val=0;
      int offset =
          17;          // exclude the first 17cm because of oversaturation
                       // issues and noisy data the Pico Flexx has in this range
      int range = 50;  // look in a tolerance range of 50cm
      for (int i = offset; i < 256; i++) {
        if (histo[tileIdx][i] > 5) {
          sum += histo[tileIdx][i];
        }
        if (i > range + offset) {
          if (histo[tileIdx][i - range] > 5) {
            sum -= histo[tileIdx][i - range];
          }
        }
        if (sum >=
            minObjSizeThresh) {  // if minObjSizeThresh is exeeded: break.
          // i now holds the depth for this tile
          val = i;
          break;
        }
      }
      // WRITE the value in the Tile Matrix:
      // Here two modification have to be done to have
      // the right visual orientation (flip, turn)
      int tileVal = (val - 255) * -1;
      // Scope for Mutex
      {
        std::lock_guard<std::mutex> lock(glob::motors.mut);
        glob::motors.tiles[(tileIdx - 8) * -1] = tileVal;
      }
    }
  }
  glob::logger.mainLogger.store("aft his");
  frameCounter++;  // counting every frame

  {
    std::lock_guard<std::mutex> lock(glob::udpServMux);
    glob::udpServer.preparePacket("frameCounter", frameCounter);
  }
  // call sending thread
  {
    std::lock_guard<std::mutex> svCondLock(glob::notifySend.mut);
    glob::notifySend.flag = true;
  }
  // glob::logger.mainLogger.store("lock");
  // glob::logger.mainLogger.store("copy");

  // glob::logger.mainLogger.store("unlock");
  // wake other thread
  glob::notifySend.cond.notify_one();

  // WRITE
  glob::logger.mainLogger.store("endProcess");
  // glob::logger.mainLogger.printAll("Receiving Frame", "us", "ms");
  // glob::logger.mainLogger.reset();
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
      if (i == 0) {
        std::lock_guard<std::mutex> lock(glob::udpServMux);
        glob::udpServer.preparePacket("drpBridge", found);
        glob::udpServer.preparePacket("drpFC", found);
        glob::udpServer.preparePacket("delivFrames", found);
      }
      i++;
    }
    /* To save from space at the end of string */
    temp = "";
  }
  // {std::lock_guard<std::mutex> lock(glob::udpServMux);
  // glob::udpServer.preparePacket("11", tenSecsDrops);}
  // tenSecsDrops += droppedAtBridge + droppedAtFC;
}

cv::Mat DepthDataUtilities::getResizedDepthImage(int incSize) {
  std::lock_guard<std::mutex> lock(glob::cvDepthImg.mut);
  cv::Mat sizedImgCopy;
  // constrain value between 1 and 9
  if (incSize <= 0 || incSize > 9) {
    incSize = 1;
  }
  int picSize = incSize * 20;
  cv::Size size(picSize, picSize);
  sizedImgCopy.create(size, CV_8UC1);
  if (glob::cvDepthImg.mat.rows != 0) {
    cv::resize(glob::cvDepthImg.mat, sizedImgCopy, size);  // resize image
  }
  return sizedImgCopy;
}
