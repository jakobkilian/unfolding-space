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

#include "Camera.hpp"
#include "Globals.hpp"
#include "MotorBoard.hpp"
#include "time.h"
#include "TimeLogger.hpp"
#include "UdpServer.hpp"

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
  {
    std::lock_guard<std::mutex> lockCoreTemp(Glob::udpServMux);
    Glob::udpServer.preparePacket("coreTemp", coreTempDouble);
  }
  tempFile.close();
}

//________________________________________________
// Mute motors before exiting the appllication
void exitApplicationMuted(__attribute__((unused)) int dummy) {
  Glob::modes.a_muted = true;
  Glob::motorBoard.muteAll();
  // delay(1);
  exit(0);
}

//**********************************************************************
//****************************** UNFOLDING *****************************
//********** This is the main part, now in a seperate thread ***********
//**********************************************************************

int unfolding() {
  Glob::a_restartUnfoldingFlag = false;
  bool threeSecondsAreOver = false;
  DepthDataListener listener;
  long timeSinceNewData;    // time passed since last "onNewData"
  int maxTimeSinceNewData;  // the longest timespan without new data since start
  bool cameraDetached;      // camera got detached

  Glob::logger.mainLogger.store("INIT");

  // Event Listener
  EventReporter eventReporter;

  royale::CameraManager manager;
  royale::Vector<royale::String> camlist;
  // this represents the main camera device object
  std::unique_ptr<royale::ICameraDevice> cameraDevice;

  // Glob::modes.a_cameraUseCase = std::move (arg);

  //_____________________INIT CAMERA____________________________________
  // check if the cam is connected before init anything
  while (camlist.empty()) {
    camlist = manager.getConnectedCameraList();
    if (!camlist.empty()) {
      cameraDevice = manager.createCamera(camlist[0]);
      break;
    }
    cout << ":";
    cout.flush();
  }
  // if camlist is not NULL
  camlist.clear();

  Glob::logger.mainLogger.store("search");
  // Mute the LRAs before ending the program by ctr + c (SIGINT)
  signal(SIGINT, exitApplicationMuted);
  signal(SIGTERM, exitApplicationMuted);

  // Setup the LRAs on the Glove (I2C Connection, Settings, Calibration, etc.)
  Glob::motorBoard.setupGlove();
  Glob::logger.mainLogger.store("glove");

  //  camera device is now available, CameraManager can be deallocated here
  if (cameraDevice == nullptr) {
    // no cameraDevice available
    cerr << "Cannot create the camera device" << endl;
    return 1;
  }
  // IMPORTANT: call the initialize method before working with camera device
  // #costly: rpi4 1000ms
  auto status = cameraDevice->initialize();
  Glob::logger.mainLogger.store("cam");

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
  if (Glob::modes.a_cameraUseCase) {
    cerr << "got the argument:" << Glob::modes.a_cameraUseCase << endl;
    auto useCaseFound = false;
    if (Glob::modes.a_cameraUseCase < useCases.size()) {
      selectedUseCaseIdx = Glob::modes.a_cameraUseCase;
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

  // Glob::listener.setLensParameters(lensParameters);

  // register a data listener
  if (cameraDevice->registerDataListener(&listener) !=
      royale::CameraStatus::SUCCESS) {
    cerr << "Error registering data listener" << endl;
    return 1;
  }
  Glob::logger.mainLogger.store("regist");
  // register a EVENT listener
  cameraDevice->registerEventListener(&eventReporter);

  //_____________________START CAPTURING_________________________________
  // start capture mode
  //#costly: rpi4 500ms
  if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS) {
    cerr << "Error starting the capturing" << endl;
    return 1;
  }
  Glob::logger.mainLogger.store("capt");
  // Reset some things
  TimeLogger startTimeLog;
  startTimeLog.store("INIT");
  cameraDetached = false;       // camera is attached and ready
  Glob::modes.a_muted = false;  // activate the vibration motors
  long lastCallImshow = millis();
  long lastCall = 0;
  long lastCallTemp = 0;
  Glob::logger.mainLogger.printAll("Initializing Unfolding", "ms", "ms");
  Glob::logger.mainLogger.reset();
  //_____________________ENDLESS LOOP_________________________________
  while (!Glob::a_restartUnfoldingFlag) {
    // Check if time since camera started capturing is bigger than 3 secs
    if (!threeSecondsAreOver) {
      if (startTimeLog.msSinceEntry(0) > 3000) {
        threeSecondsAreOver = true;
      }
    }

    if (!Glob::royalStats.a_isCalibRunning) {
      // only do all of this stuff when the camera is attached
      if (!cameraDetached) {
        royale::String id;
        royale::String name;
        bool tempisCalibrated;
        bool tempisConnected;
        bool tempisCapturing;
        uint16_t maxSensorWidth;
        uint16_t maxSensorHeight;
        // time passed since last OnNewData
        timeSinceNewData = Glob::logger.newDataLog.msSinceEntry(0);
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
          status = cameraDevice->getCameraInfo(cameraInfo);
          status = cameraDevice->getMaxSensorHeight(maxSensorHeight);
          status = cameraDevice->getMaxSensorWidth(maxSensorWidth);
          status = cameraDevice->getCameraName(name);
          status = cameraDevice->getId(id);
          status = cameraDevice->isCalibrated(tempisCalibrated);
          status = cameraDevice->isConnected(tempisConnected);
          status = cameraDevice->isCapturing(tempisCapturing);

          Glob::royalStats.a_isCalibrated = tempisCalibrated;
          Glob::royalStats.a_isConnected = tempisConnected;
          Glob::royalStats.a_isCapturing = tempisCapturing;
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
          if (Glob::royalStats.a_isConnected == 0 &&
              Glob::royalStats.a_isCapturing == 1) {
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
              Glob::modes.a_muted = true;
              Glob::motorBoard.muteAll();
              cameraDetached = true;
              Glob::a_restartUnfoldingFlag = true;
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
              Glob::royalStats.a_libraryCrashCounter++;
              // stop writing new values to the LRAs
              Glob::modes.a_muted = true;
              // mute all LRAs
              Glob::motorBoard.muteAll();
              // go to the beginning and find camera again
              Glob::a_restartUnfoldingFlag = true;
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
  Glob::modes.a_muted = true;
  // mute all motors
  Glob::motorBoard.muteAll();
  return 0;
}

// TODO: maybe this is not the best way to handle this?
class mainThreadWrapper {
 public:
  // Sending the data out at specified moments when there is nothing else to do
  void runUdpSend() { Glob::udpService.run(); }
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
    Glob::udpService.stop();
  }
  std::thread runUnfoldingThread() {
    return std::thread([=] { runUnfolding(); });
  }

  // Processing the Data, Creating Depth Image, Histogram and 9-Tiles Array
  void runCopyDepthData() {
    DepthDataUtilities ddProcessor;
    while (1) {
      std::unique_lock<std::mutex> pdCondLock(Glob::notifyProcess.mut);
      Glob::notifyProcess.cond.wait(pdCondLock,
                                    [] { return Glob::notifyProcess.flag; });
      ddProcessor.processData();
      Glob::notifyProcess.flag = false;
    }
  }
  std::thread runCopyDepthDataThread() {
    return std::thread([=] { runCopyDepthData(); });
  }

  // Sending the Data to the glove (Costly due to register writing via i2c)
  void runSendDepthData() {
    while (1) {
      {
        std::unique_lock<std::mutex> svCondLock(Glob::notifySend.mut);
        Glob::notifySend.cond.wait(svCondLock,
                                   [] { return Glob::notifySend.flag; });
      }
      // IF in regular mode
      if (!Glob::modes.a_testMode) {
        std::lock_guard<std::mutex> lockMotorTiles(Glob::motors.mut);
        Glob::motorBoard.sendValuesToGlove(Glob::motors.tiles, 9);
        const int size =
            sizeof(Glob::motors.testTiles) / sizeof(Glob::motors.testTiles[0]);
        std::vector<unsigned char> vect;
        for (int i = 0; i < size; i++) {
          unsigned char tmpChar = Glob::motors.tiles[i];
          vect.push_back(tmpChar);
        }
        {
          std::lock_guard<std::mutex> lockSendMotors(Glob::udpServMux);
          Glob::udpServer.preparePacket("motors", vect);
          Glob::udpServer.prepareImage();
        }
      }  // IF in test mode
      else {
        std::lock_guard<std::mutex> lockMotorTiles2(Glob::motors.mut);
        Glob::motorBoard.sendValuesToGlove(Glob::motors.testTiles, 9);
        const int size =
            sizeof(Glob::motors.testTiles) / sizeof(Glob::motors.testTiles[0]);
        std::vector<unsigned char> vect;

        for (int i = 0; i < size; i++) {
          unsigned char tmpChar = Glob::motors.testTiles[i];
          vect.push_back(tmpChar);
        }
        {
          std::lock_guard<std::mutex> lockSendMotors2(Glob::udpServMux);
          Glob::udpServer.preparePacket("motors", vect);
        }
      }

      // Send depth image no matter if test mode or not.
      {
        std::lock_guard<std::mutex> lockPrepareImage(Glob::udpServMux);
        Glob::udpServer.prepareImage();
      }

      // TODO: sollte gleichzeitig zu send ausgeführt werden und nicht
      // danach...
      bool tempisConnected = Glob::royalStats.a_isConnected;
      bool tempisCapturing = Glob::royalStats.a_isCapturing;
      int tempCounter = Glob::royalStats.a_libraryCrashCounter;
      bool tempMuted = Glob::modes.a_muted;
      bool tempTest = Glob::modes.a_testMode;
      int lockfail = Glob::a_lockFailCounter;

      {
        std::lock_guard<std::mutex> lockSendValues(Glob::udpServMux);
        Glob::udpServer.preparePacket("isConnected", tempisConnected);
        Glob::udpServer.preparePacket("isCapturing", tempisCapturing);
        Glob::udpServer.preparePacket("libCrashes", tempCounter);
        Glob::udpServer.preparePacket("isMuted", tempMuted);
        Glob::udpServer.preparePacket("isTestMode", tempTest);
        Glob::udpServer.preparePacket("lockFails", lockfail);
      }
      {
        std::unique_lock<std::mutex> svCondLock(Glob::notifySend.mut);
        Glob::notifySend.flag = false;
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
int main() {
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
