/*
 * File: main.cpp
 * Author: Jakob Kilian
 * Date: 26.09.19
 * Info: Containts Main (endless) loop, UDP Handling and Analysis stuff
 * Project: unfoldingspace.jakobkilian.de
 */

//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include <signal.h>

#include <algorithm>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <chrono>
#include <ctime>
#include <iostream>
#include <royale/IEvent.hpp>
#include <sstream>
#include <string>
#include <thread>

#include "camera.hpp"
#include "globals.hpp"
#include "glove.hpp"
#include "poti.hpp"
#include "time.h"
#include "timelog.hpp"
#include "udp.hpp"

using boost::asio::ip::udp;
using std::cerr;
using std::cout;
using std::endl;
using namespace std::chrono;

//----------------------------------------------------------------------
// SETTINGS
//----------------------------------------------------------------------
// Does the system use a poti?
bool potiAv = 0;
// Does the system use the old DRV-Breakoutboards or the new detachable DRV-PCB?
bool detachableDRV = 0;

//----------------------------------------------------------------------
// DECLARATIONS AND VARIABLES
//----------------------------------------------------------------------

// Todo: global stuff
long timeSinceLastNewData;  // time passed since last "onNewData"
int fpsFromCam;             // wich royal use case is used? (how many fps?)
int currentKey = 0;         //
int longestTimeNoData;      // the longest timespan without new data since start
bool cameraDetached;        // camera got detached
long cameraStartTime;       // timestamp when camera started capturing

DepthDataListener listener;

//----------------------------------------------------------------------
// OTHER FUNCTIONS AND CLASSES
//----------------------------------------------------------------------

//________________________________________________
// Royale Event Listener reports dropped frames as string.
// This functions extracts the number of frames that got lost at Bridge/FC.
// I believe, that dropped frames cause instability – PMDtec confirmed this
void extractDrops(royale::String str) {
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
      if (i == 0) glob::udpSendServer.preparePacket("9", found);
      if (i == 1) glob::udpSendServer.preparePacket("10", found);
      if (i == 2) glob::udpSendServer.preparePacket("12", found);
      i++;
    }
    /* To save from space at the end of string */
    temp = "";
  }
  // glob::udpSendServer.preparePacket("11", tenSecsDrops);
  // tenSecsDrops += droppedAtBridge + droppedAtFC;
}

//________________________________________________
// Gets called by Royale irregularily.
// Holds the camera state, errors and info about drops
class EventReporter : public royale::IEventListener {
 public:
  virtual ~EventReporter() = default;

  virtual void onEvent(std::unique_ptr<royale::IEvent> &&event) override {
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
};

//________________________________________________
// Read out the core temperature and save it in coreTempDouble
void getCoreTemp() {
  std::string coreTemp;
  std::ifstream tempFile("/sys/class/thermal/thermal_zone0/temp");
  tempFile >> coreTemp;
  coreTemp = coreTemp.insert(2, 1, '.');
  float coreTempDouble = std::stod(coreTemp);
  glob::udpSendServer.preparePacket("coreTemp", coreTempDouble);
  tempFile.close();
}

//________________________________________________
// When an error occurs(camera gets detached): prevent freezing, but mute all
void endMuted(int dummy) {
  glob::isMuted = true;
  delay(1);
  muteAll();
  delay(1);
  exit(0);
}

//**********************************************************************
//****************************** UNFOLDING *****************************
//********** This is the main part, now in a seperate thread ***********
//**********************************************************************
timelog mainTimeLog(20);

int unfolding() {
  mainTimeLog.store("-");
  // This is the data listener which will receive callbacks.  It's declared
  // before the cameraDevice so that, if this function exits with a 'return'
  // statement while  camera is still capturing, it will still be in scope
  // until the cameraDevice's destructor implicitly de-registers  listener.

  // Event Listener
  EventReporter eventReporter;

  royale::CameraManager manager;
  royale::Vector<royale::String> camlist;
  // this represents the main camera device object
  std::unique_ptr<royale::ICameraDevice> cameraDevice;
  uint commandLineUseCase = 1;
  // commandLineUseCase = std::move (arg);
searchCam:
  //_____________________INIT CAMERA____________________________________
  // check if the cam is connected before init anything
  while (camlist.empty()) {
    camlist = manager.getConnectedCameraList();
    if (!camlist.empty()) {
      cameraDevice = manager.createCamera(camlist[0]);
      break;
    }
    cout << ".";
    cout.flush();
  }
  // if camlist is not NULL
  camlist.clear();

  mainTimeLog.store("search");
  // Mute the LRAs before ending the program by ctr + c (SIGINT)
  signal(SIGINT, endMuted);
  signal(SIGTERM, endMuted);
  // initialize boost library
  // boostInit();

  // Setup Connection to Digispark Board for Poti-Reads
  if (potiAv) initPoti();

  // Setup the LRAs on the Glove (I2C Connection, Settings, Calibration, etc.)
  setupGlove();
  mainTimeLog.store("glove");

  //  camera device is now available, CameraManager can be deallocated here
  if (cameraDevice == nullptr) {
    // no cameraDevice available
    cerr << "Cannot create the camera device" << endl;
    return 1;
  }
  // IMPORTANT: call the initialize method before working with camera device
  // #costly: rpi4 1000ms
  auto status = cameraDevice->initialize();
  mainTimeLog.store("cam");

  if (status != royale::CameraStatus::SUCCESS) {
    cerr << "Cannot initialize the camera device, error string : "
         << getErrorString(status) << endl;
    return 1;
  }
  royale::Vector<royale::String> useCases;
  auto usecaseStatus = cameraDevice->getUseCases(useCases);

  if (usecaseStatus != royale::CameraStatus::SUCCESS || useCases.empty()) {
    cerr << "No use cases are available" << endl;
    cerr << "getUseCases() returned: " << getErrorString(usecaseStatus) << endl;
    return 1;
  }
  cerr << useCases << endl;
  // choose a use case
  uint selectedUseCaseIdx = 0u;
  if (commandLineUseCase) {
    cerr << "got the argument:" << commandLineUseCase << endl;
    auto useCaseFound = false;
    if (commandLineUseCase >= 0 && commandLineUseCase < useCases.size()) {
      uint8_t fpsUseCases[6] = {0, 10, 15, 25, 35, 45};  // pico flexx specs
      selectedUseCaseIdx = commandLineUseCase;
      fpsFromCam = fpsUseCases[commandLineUseCase];
      useCaseFound = true;
    }

    if (!useCaseFound) {
      cerr << "Error: the chosen use case is not supported by this camera"
           << endl;
      cerr << "A list of supported use cases is printed by sampleCameraInfo"
           << endl;
      return 1;
    }
  } else {
    cerr << "Here: autousecase id" << endl;
    // choose the first use case
    selectedUseCaseIdx = 0;
  }
  // set an operation mode
  if (cameraDevice->setUseCase(useCases.at(selectedUseCaseIdx)) !=
      royale::CameraStatus::SUCCESS) {
    cerr << "Error setting use case" << endl;
    return 1;
  }
  // retrieve the lens parameters from Royale
  royale::LensParameters lensParameters;
  status = cameraDevice->getLensParameters(lensParameters);
  if (status != royale::CameraStatus::SUCCESS) {
    cerr << "Can't read out the lens parameters" << endl;
    return 1;
  }

  // listener.setLensParameters(lensParameters);

  // register a data listener
  if (cameraDevice->registerDataListener(&listener) !=
      royale::CameraStatus::SUCCESS) {
    cerr << "Error registering data listener" << endl;
    return 1;
  }
  mainTimeLog.store("regist");
  // register a EVENT listener
  cameraDevice->registerEventListener(&eventReporter);

  //_____________________START CAPTURING_________________________________
  // start capture mode
  //#costly: rpi4 500ms
  if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS) {
    cerr << "Error starting the capturing" << endl;
    return 1;
  }
  mainTimeLog.store("capt");
  // Reset some things
  cameraStartTime = millis();  // set timestamp for capturing start
  cameraDetached = false;      // camera is attached and ready
  glob::isMuted = false;       // activate the vibration motors
  long lastCallImshow = millis();
  long lastCall = 0;
  long lastCallTemp = 0;
  long lastCallPoti = millis();
  mainTimeLog.print("Initializing Unfolding", "ms", "ms");
  //_____________________ENDLESS LOOP_________________________________
  while (currentKey != 27)  //...until Esc is pressed
  {
    if (!glob::royalStats.isCalibRunning) {
      // only do all of this stuff when the camera is attached
      if (!cameraDetached) {
        royale::String id;
        royale::String name;
        uint16_t maxSensorWidth;
        uint16_t maxSensorHeight;
        // time passed since LastNewData
        timeSinceLastNewData = millis() - lastNewData;
        if (longestTimeNoData < timeSinceLastNewData) {
          longestTimeNoData = timeSinceLastNewData;
        }

        // do this every 66ms (15 fps)
        if (millis() - lastCallImshow > 66) {
          // update test motor vals

          lastCallImshow = millis();
          // Get all the data of the royal lib to see if camera is working
          royale::Vector<royale::Pair<royale::String, royale::String>>
              cameraInfo;
          auto status = cameraDevice->getCameraInfo(cameraInfo);
          status = cameraDevice->getMaxSensorHeight(maxSensorHeight);
          status = cameraDevice->getMaxSensorWidth(maxSensorWidth);
          status = cameraDevice->getCameraName(name);
          status = cameraDevice->getId(id);
          status = cameraDevice->isCalibrated(glob::royalStats.isCalibrated);
          status = cameraDevice->isConnected(glob::royalStats.isConnected);
          status = cameraDevice->isCapturing(glob::royalStats.isCapturing);

          // do this every 50ms (20 fps)
          if (millis() - lastCallPoti > 100) {
            lastCallPoti = millis();
            if (glob::isTestMode) {
              {
                std::lock_guard<std::mutex> lock(glob::m_testTiles);
                sendValuesToGlove(glob::testTiles, 9);
              }
            }

            if (potiAv) updatePoti();

            // calc fps
            int secondsSinceReset = ((millis() - 0) / 1000);
            // if (secondsSinceReset > 0) {
            //   fps = frameCounter / secondsSinceReset;
            // }
            if (frameCounter > 999) {
              frameCounter = 0;
              // 0 = millis();
            }

            // printf("time since last new data: %i ms \n",
            // timeSinceLastNewData); printf("No of library crashes: %i times
            // \n", libraryCrashCounter); printf("longest time with no new data
            // was: %i \n", longestTimeNoData); printf("temp.: \t%.1f°C\n",
            // coreTempDouble); printf("drops:\t%i | %i\t deliver:\t%i \t drops
            // in last 10sec: %i\n",
            //        droppedAtBridge, droppedAtFC, deliveredFrames,
            //        tenSecsDrops);
            // printf("frame:\t %i \t time:\t %i \t fps: %.1f \n", frameCounter,
            //        secondsSinceReset, fps);
            // printf("cycle:\t %ims \t pause: %ims \n", globalCycleTime,
            //        globalPauseTime);
            // printf("Range:\t %.1f m\n\n\n", maxDepth);
            // printOutput();
          }

          // do this every 5000ms (every 1 seconds)
          if (millis() - lastCallTemp > 1000) {
            lastCallTemp = millis();
            getCoreTemp();  // read raspi's core temperature
          }

          // do this every 5000ms (every 1 seconds)
          if (millis() - lastCall > 10000) {
            lastCall = millis();
            // tenSecsDrops = 0;
          }
        }

        // RESTART WHEN CAMERA IS UNPLUGGED
        // Ignore first 3secs
        if ((millis() - cameraStartTime) > 3000) {
          // connected but still capturing -> unplugged!
          if (glob::royalStats.isConnected == 0 &&
              glob::royalStats.isCapturing == 1) {
            if (cameraDetached == false) {
              cout << "________________________________________________" << endl
                   << endl;
              cout << "Camera Detached! Reinitialize Camera and Listener"
                   << endl
                   << endl;
              cout << "________________________________________________" << endl
                   << endl;
              cout << "Searching for 3D camera in loop" << endl;
              // stop writing new values to the LRAs
              glob::isMuted = true;
              // mute all LRAs
              muteAll();
              cameraDetached = true;
            }
            royale::CameraManager manager;
            royale::Vector<royale::String> camlist;
            while (camlist.empty()) {
              camlist = manager.getConnectedCameraList();
              if (!camlist.empty()) {
                camlist.clear();
                goto searchCam;  // jump back to the beginning
              }
              cout << ".";
              cout.flush();
            }
          }
        }

        if ((millis() - cameraStartTime) > 3000) {  // ignore the first 3 secs
          if (cameraDetached == false) {        // if camera should be there...
            if (timeSinceLastNewData > 4000) {  // but there is no frame for 4s
              cout << "________________________________________________" << endl
                   << endl;
              cout << "Library Crashed! Reinitialize Camera and Listener. last "
                      "new "
                      "frame:  "
                   << timeSinceLastNewData << endl
                   << endl;
              cout << "________________________________________________" << endl
                   << endl;
              glob::royalStats.libraryCrashCounter++;
              // stop writing new values to the LRAs
              glob::isMuted = true;
              // mute all LRAs
              muteAll();
              // go to the beginning and find camera again
              goto searchCam;
            }
          }
        }
      }
    }
  }
  //_____________________END OF PROGRAMM________________________________
  // stop capturing mode
  if (cameraDevice->stopCapture() != royale::CameraStatus::SUCCESS) {
    cerr << "Error stopping the capturing" << endl;
    return 1;
  }
  glob::isMuted = true;
  // mute all motors
  muteAll();
  return 0;
}
// TODO: maybe this is not the best way to handle this?
class mainThreadWrapper {
 public:
  // Sending the data out at specified moments when there is nothing else to do
  void runUdpSend() { glob::udpSendService.run(); }
  std::thread runUdpSendThread() {
    return std::thread([=] { runUdpSend(); });
  }

  // Run the Main Code with the endless loop
  void runUnfolding() {
    int unfReturn = unfolding();
    printf("Unfolding Thread has been exited with a #%i \n", unfReturn);
    printf("Stopping the UDP server now... \n");
    // Todo: stoppen notwendig? Was ist mein Stop-plan?
    glob::udpSendService.stop();
  }
  std::thread runUnfoldingThread() {
    return std::thread([=] { runUnfolding(); });
  }

  // Processing the Data, Creating Depth Image, Histogram and 9-Tiles Array
  void runCopyDepthData() {
    while (1) {
      std::unique_lock<std::mutex> pdCondLock(pdCondMutex);
      pdCond.wait(pdCondLock, [] { return pdFlag; });
      listener.processData();
      pdFlag = false;
    }
  }
  std::thread runCopyDepthDataThread() {
    return std::thread([=] { runCopyDepthData(); });
  }

  // Sending the Data to the glove (Costly due to register writing via i2c)
  void runSendDepthData() {
    while (1) {
      std::unique_lock<std::mutex> svCondLock(svCondMutex);
      svCond.wait(svCondLock, [] { return svFlag; });
      sendValuesToGlove(glob::tiles, 9);

      // Send motor vals or testvals
      if (!glob::isTestMode) {
        std::lock_guard<std::mutex> lock(glob::m_tiles);
        const int size = sizeof(glob::testTiles) / sizeof(glob::testTiles[0]);
        std::vector<unsigned char> vect;
        for (int i = 0; i < size; i++) {
          unsigned char tmpChar = glob::tiles[i];
          vect.push_back(tmpChar);
        }
        glob::udpSendServer.preparePacket("motors", vect);
      } else {
        std::lock_guard<std::mutex> lock(glob::m_testTiles);
        const int size = sizeof(glob::testTiles) / sizeof(glob::testTiles[0]);
        std::vector<unsigned char> vect;

        for (int i = 0; i < size; i++) {
          unsigned char tmpChar = glob::testTiles[i];
          vect.push_back(tmpChar);
        }
        glob::udpSendServer.preparePacket("motors", vect);
      }
      glob::udpSendServer.prepareImage();

      // TODO: sollte gleichzeitig zu send ausgeführt werden und nicht danach...
      glob::udpSendServer.preparePacket("6", glob::royalStats.isConnected);
      glob::udpSendServer.preparePacket("7", glob::royalStats.isCapturing);
      glob::udpSendServer.preparePacket("8",
                                        glob::royalStats.libraryCrashCounter);
      glob::udpSendServer.preparePacket("15", glob::isMuted);
      glob::udpSendServer.preparePacket("16", glob::isTestMode);

      svFlag = false;
    }
  }
  std::thread runSendDepthDataThread() {
    return std::thread([=] { runSendDepthData(); });
  }
};

//----------------------------------------------------------------------
// MAIN LOOP
//----------------------------------------------------------------------
int main(int argc, char *argv[]) {
  // create thread wrapper instance and the threads
  mainThreadWrapper *w = new mainThreadWrapper();
  std::thread udpSendTh = w->runUdpSendThread();
  std::thread unfTh = w->runUnfoldingThread();
  std::thread ddCopyTh = w->runCopyDepthDataThread();
  std::thread ddSendTh = w->runSendDepthDataThread();
  udpSendTh.join();
  unfTh.join();
  ddCopyTh.join();
  ddSendTh.join();
  return 0;
}
