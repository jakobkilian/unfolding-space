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

//________________________________________________
// Read out the core temperature and save it in coreTempDouble
void getCoreTemp() {
  std::string coreTemp;
  std::ifstream tempFile("/sys/class/thermal/thermal_zone0/temp");
  tempFile >> coreTemp;
  coreTemp = coreTemp.insert(2, 1, '.');
  float coreTempDouble = std::stod(coreTemp);
  glob::udpServer.preparePacket("coreTemp", coreTempDouble);
  tempFile.close();
}

//________________________________________________
// Mute motors before exiting the appllication
void exitApplicationMuted(int dummy) {
  glob::modes.a_muted = true;
  muteAll();
  // delay(1);
  exit(0);
}

//**********************************************************************
//****************************** UNFOLDING *****************************
//********** This is the main part, now in a seperate thread ***********
//**********************************************************************

int unfolding() {
  glob::a_restartUnfoldingFlag = false;
  bool threeSecondsAreOver = false;
  DepthDataListener listener;
  long timeSinceNewData;    // time passed since last "onNewData"
  int fpsFromCam;           // wich royal use case is used? (how many fps?)
  int maxTimeSinceNewData;  // the longest timespan without new data since start
  bool cameraDetached;      // camera got detached

  glob::logger.mainLogger.store("INIT");

  // Event Listener
  EventReporter eventReporter;

  royale::CameraManager manager;
  royale::Vector<royale::String> camlist;
  // this represents the main camera device object
  std::unique_ptr<royale::ICameraDevice> cameraDevice;

  // glob::modes.a_cameraUseCase = std::move (arg);

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

  glob::logger.mainLogger.store("search");
  // Mute the LRAs before ending the program by ctr + c (SIGINT)
  signal(SIGINT, exitApplicationMuted);
  signal(SIGTERM, exitApplicationMuted);
  // initialize boost library
  // boostInit();

  // Setup Connection to Digispark Board for Poti-Reads
  if (glob::potiStats.a_potAvail) initPoti();

  // Setup the LRAs on the Glove (I2C Connection, Settings, Calibration, etc.)
  setupGlove();
  glob::logger.mainLogger.store("glove");

  //  camera device is now available, CameraManager can be deallocated here
  if (cameraDevice == nullptr) {
    // no cameraDevice available
    cerr << "Cannot create the camera device" << endl;
    return 1;
  }
  // IMPORTANT: call the initialize method before working with camera device
  // #costly: rpi4 1000ms
  auto status = cameraDevice->initialize();
  glob::logger.mainLogger.store("cam");

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
  if (glob::modes.a_cameraUseCase) {
    cerr << "got the argument:" << glob::modes.a_cameraUseCase << endl;
    auto useCaseFound = false;
    if (glob::modes.a_cameraUseCase >= 0 &&
        glob::modes.a_cameraUseCase < useCases.size()) {
      uint8_t fpsUseCases[6] = {0, 10, 15, 25, 35, 45};  // pico flexx specs
      selectedUseCaseIdx = glob::modes.a_cameraUseCase;
      fpsFromCam = fpsUseCases[glob::modes.a_cameraUseCase];
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

  // glob::listener.setLensParameters(lensParameters);

  // register a data listener
  if (cameraDevice->registerDataListener(&listener) !=
      royale::CameraStatus::SUCCESS) {
    cerr << "Error registering data listener" << endl;
    return 1;
  }
  glob::logger.mainLogger.store("regist");
  // register a EVENT listener
  cameraDevice->registerEventListener(&eventReporter);

  //_____________________START CAPTURING_________________________________
  // start capture mode
  //#costly: rpi4 500ms
  if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS) {
    cerr << "Error starting the capturing" << endl;
    return 1;
  }
  glob::logger.mainLogger.store("capt");
  // Reset some things
  timelog startTimeLog;
  startTimeLog.store("INIT");
  cameraDetached = false;       // camera is attached and ready
  glob::modes.a_muted = false;  // activate the vibration motors
  long lastCallImshow = millis();
  long lastCall = 0;
  long lastCallTemp = 0;
  long lastCallPoti = millis();
  glob::logger.mainLogger.printAll("Initializing Unfolding", "ms", "ms");
  glob::logger.mainLogger.reset();
  //_____________________ENDLESS LOOP_________________________________
  while (!glob::a_restartUnfoldingFlag) {
    // Check if time since camera started capturing is bigger than 3 secs
    if (!threeSecondsAreOver) {
      if (startTimeLog.msSinceEntry(0) > 3000) {
        threeSecondsAreOver = true;
      }
    }

    if (!glob::royalStats.a_isCalibRunning) {
      // only do all of this stuff when the camera is attached
      if (!cameraDetached) {
        royale::String id;
        royale::String name;
        bool tempisCalibrated;
        bool tempisConnected;
        bool tempisCapturing;
        uint16_t maxSensorWidth;
        uint16_t maxSensorHeight;
        // time passed since LastNewData
        timeSinceNewData = glob::logger.newDataLog.msSinceEntry(0);
        if (maxTimeSinceNewData < timeSinceNewData) {
          maxTimeSinceNewData = timeSinceNewData;
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
          status = cameraDevice->isCalibrated(tempisCalibrated);
          status = cameraDevice->isConnected(tempisConnected);
          status = cameraDevice->isCapturing(tempisCapturing);

          glob::royalStats.a_isCalibrated = tempisCalibrated;
          glob::royalStats.a_isConnected = tempisConnected;
          glob::royalStats.a_isCapturing = tempisCapturing;
        }

        // do this every 50ms (20 fps)
        if (millis() - lastCallPoti > 100) {
          lastCallPoti = millis();

          if (glob::potiStats.a_potAvail) updatePoti();

          // calc fps
          int secondsSinceReset = ((millis() - 0) / 1000);
          // if (secondsSinceReset > 0) {
          //   fps = frameCounter / secondsSinceReset;
          // }
          if (frameCounter > 999) {
            frameCounter = 0;
            // 0 = millis();
          }
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

        // Ignore first 3secs
        if (threeSecondsAreOver) {
          // RESTART WHEN CAMERA IS UNPLUGGED
          // connected but still capturing -> unplugged!
          if (glob::royalStats.a_isConnected == 0 &&
              glob::royalStats.a_isCapturing == 1) {
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
              glob::modes.a_muted = true;
              muteAll();
              cameraDetached = true;
            }
            royale::CameraManager manager;
            royale::Vector<royale::String> camlist;
            while (camlist.empty()) {
              camlist = manager.getConnectedCameraList();
              if (!camlist.empty()) {
                camlist.clear();
                glob::a_restartUnfoldingFlag = true;
              }
              cout << ".";
              cout.flush();
            }
          }
          if (cameraDetached == false) {    // if camera should be there...
            if (timeSinceNewData > 4000) {  // but there is no frame for 4s
              cout << "________________________________________________" << endl
                   << endl;
              cout << "Library Crashed! Reinitialize Camera and Listener. last "
                      "new "
                      "frame:  "
                   << timeSinceNewData << endl
                   << endl;
              cout << "________________________________________________" << endl
                   << endl;
              glob::royalStats.a_libraryCrashCounter++;
              // stop writing new values to the LRAs
              glob::modes.a_muted = true;
              // mute all LRAs
              muteAll();
              // go to the beginning and find camera again
              glob::a_restartUnfoldingFlag = true;
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
  glob::modes.a_muted = true;
  // mute all motors
  muteAll();
  return 0;
}
// TODO: maybe this is not the best way to handle this?
class mainThreadWrapper {
 public:
  // Sending the data out at specified moments when there is nothing else to do
  void runUdpSend() { glob::udpService.run(); }
  std::thread runUdpSendThread() {
    return std::thread([=] { runUdpSend(); });
  }

  // Run the Main Code with the endless loop
  void runUnfolding() {
    int unfReturn;
    while (true) {
      unfReturn = unfolding();
    }
    printf("Unfolding Thread has been exited with a #%i \n", unfReturn);
    printf("Stopping the UDP server now... \n");
    // Todo: stoppen notwendig? Was ist mein Stop-plan?
    glob::udpService.stop();
  }
  std::thread runUnfoldingThread() {
    return std::thread([=] { runUnfolding(); });
  }

  // Processing the Data, Creating Depth Image, Histogram and 9-Tiles Array
  void runCopyDepthData() {
    DepthDataUtilities ddProcessor;
    while (1) {
      std::unique_lock<std::mutex> pdCondLock(glob::notifyProcess.mut);
      glob::notifyProcess.cond.wait(pdCondLock,
                                    [] { return glob::notifyProcess.flag; });
      ddProcessor.processData();
      glob::notifyProcess.flag = false;
    }
  }
  std::thread runCopyDepthDataThread() {
    return std::thread([=] { runCopyDepthData(); });
  }

  // Sending the Data to the glove (Costly due to register writing via i2c)
  void runSendDepthData() {
    while (1) {
      {
        std::unique_lock<std::mutex> svCondLock(glob::notifySend.mut);
        glob::notifySend.cond.wait(svCondLock,
                                   [] { return glob::notifySend.flag; });
      }
      // Send motor vals or testvals
      if (!glob::modes.a_testMode) {
        std::lock_guard<std::mutex> lock(glob::motors.mut);
        sendValuesToGlove(glob::motors.tiles, 9);
        const int size =
            sizeof(glob::motors.testTiles) / sizeof(glob::motors.testTiles[0]);
        std::vector<unsigned char> vect;
        for (int i = 0; i < size; i++) {
          unsigned char tmpChar = glob::motors.tiles[i];
          vect.push_back(tmpChar);
        }
        glob::udpServer.preparePacket("motors", vect);
      } else {
        std::lock_guard<std::mutex> lock(glob::motors.mut);
        sendValuesToGlove(glob::motors.testTiles, 9);
        const int size =
            sizeof(glob::motors.testTiles) / sizeof(glob::motors.testTiles[0]);
        std::vector<unsigned char> vect;

        for (int i = 0; i < size; i++) {
          unsigned char tmpChar = glob::motors.testTiles[i];
          vect.push_back(tmpChar);
        }
        glob::udpServer.preparePacket("motors", vect);
      }
      glob::udpServer.prepareImage();
      // TODO: sollte gleichzeitig zu send ausgeführt werden und nicht danach...
      bool tempisConnected = glob::royalStats.a_isConnected;
      bool tempisCapturing = glob::royalStats.a_isCapturing;
      int tempCounter = glob::royalStats.a_libraryCrashCounter;
      bool tempMuted = glob::modes.a_muted;
      bool tempTest = glob::modes.a_testMode;
      int lockfail = glob::a_lockFailCounter;

      glob::udpServer.preparePacket("isConnected", tempisConnected);
      glob::udpServer.preparePacket("isCapturing", tempisCapturing);
      glob::udpServer.preparePacket("libCrashes", tempCounter);
      glob::udpServer.preparePacket("isMuted", tempMuted);
      glob::udpServer.preparePacket("isTestMode", tempTest);
      glob::udpServer.preparePacket("lockFails", lockfail);
      /*
       unsigned char m_Test[20] = "Hello World";
       glob::udpServer.preparePacket("debug", m_Test);
 */ {
        std::unique_lock<std::mutex> svCondLock(glob::notifySend.mut);
        glob::notifySend.flag = false;
      }
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
