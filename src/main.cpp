/* INFO
 * Initilises all components and persists in endless loop for mainting app and
 * doing time based tasks. Creates 4 main threads (see readme).
 */
//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include <signal.h>

#include <algorithm>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <chrono>
#include <ctime>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <thread>
namespace po = boost::program_options;

#include "Camera.hpp"
#include "Globals.hpp"
#include "MotorBoard.hpp"
#include "TimeLogger.hpp"
#include "UdpServer.hpp"
#include "time.h"

using boost::asio::ip::udp;
using std::cerr;
using std::cout;
using std::endl;
using namespace std::chrono;

#ifndef VERSION
#define VERSION "unknown"
// VERSION is defined by the Makefile
#endif

//________________________________________________
// Check Internet Connection
bool isInternetConnected() {
  static const char *hostname = "google.com";
  struct hostent *hostinfo;
  hostinfo = gethostbyname(hostname);
  bool connected;
  if (hostinfo == NULL)
    connected = false;
  else
    connected = true;
  return connected;
}

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
  {
    std::lock_guard<std::mutex> lockMotorTiles(Glob::motors.mut);
    Glob::motorBoard.muteAll();
  }
  Glob::led1.off();
  Glob::led2.off();
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
  long timeSinceNewData;   // time passed since last "onNewData"
  int maxTimeSinceNewData; // the longest timespan without new data since start
  bool cameraDetached;     // camera got detached

  // Init LEDs
  Glob::led1.init();
  Glob::led2.init();
  Glob::led1.off();
  Glob::led2.off();
  // Turn on green init LED
  Glob::led1.setG(1);

  Glob::logger.mainLogger.store("INIT");

  // Event Listener
  EventReporter eventReporter;

  royale::CameraManager manager;
  royale::Vector<royale::String> camlist;
  // this represents the main camera device object
  std::unique_ptr<royale::ICameraDevice> cameraDevice;

  //_____________________INIT CAMERA____________________________________
  // check if the cam is connected before init anything
  bool cameraSearchBlink = false;
  while (camlist.empty()) {
    cameraSearchBlink = !cameraSearchBlink;
    camlist = manager.getConnectedCameraList();
    if (!camlist.empty()) {
      cameraDevice = manager.createCamera(camlist[0]);
      break;
    }
    Glob::led1.setG(0);
    cout << ":";
    cout.flush();
    delay(100);
    if (cameraSearchBlink)
      Glob::led1.setB(1);
    else
      Glob::led1.setB(0);
  }
  // Turn Off Camera Search blink
  Glob::led1.setB(0);
  Glob::led1.setG(1);

  // if camlist is not NULL
  camlist.clear();

  Glob::logger.mainLogger.store("search");
  // Mute the LRAs before ending the program by ctr + c (SIGINT)
  signal(SIGINT, exitApplicationMuted);
  signal(SIGTERM, exitApplicationMuted);

  // Setup the LRAs on the Glove (I2C Connection, Settings, Calibration, etc.)
  {
    std::lock_guard<std::mutex> lockMotorTiles(Glob::motors.mut);
    Glob::motorBoard.setupGlove();
  }
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

  // JUST FOR TESTING IF THE LSM DEVICE IS THERE
  {
    std::lock_guard<std::mutex> lockimu(Glob::imuMux);
    Glob::imu.init();
  }

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
  cameraDetached = false;      // camera is attached and ready
  Glob::modes.a_muted = false; // activate the vibration motors
  long lastCallImshow = millis();
  long lastCall = millis() - 10000;
  long lastCallTemp = 0;
  // Turn off green init LED
  Glob::led1.setG(0);
  // Glob::logger.mainLogger.printAll("Initializing Unfolding", "ms", "ms");
  Glob::logger.mainLogger.reset();

  bool lastMuted = 0;
  bool statusBlink = 0;
  bool internetConnected = 0;
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
        bool tempisConnected;
        bool tempisCapturing;
        // time passed since last OnNewData
        timeSinceNewData = Glob::logger.newDataLog.msSinceEntry(0);
        if (maxTimeSinceNewData < timeSinceNewData) {
          maxTimeSinceNewData = timeSinceNewData;
        }

        // do this every 66ms (15 fps)
        if (millis() - lastCallImshow > 66) {
          // update LEDs
          if (Glob::modes.a_muted != lastMuted) {
            lastMuted = Glob::modes.a_muted;
            if (lastMuted) {
              Glob::led1.setDimR(1);
            } else {
              Glob::led1.setDimR(0);
            }
          }

          // update test motor vals
          lastCallImshow = millis();
          // Get all the data of the royal lib to see if camera is working
          status = cameraDevice->isConnected(tempisConnected);
          status = cameraDevice->isCapturing(tempisCapturing);

          Glob::royalStats.a_isConnected = tempisConnected;
          Glob::royalStats.a_isCapturing = tempisCapturing;
        }
        // do this every 5000ms (every 1 seconds)
        if (millis() - lastCallTemp > 1000) {
          if (internetConnected) {
            Glob::led2.setDimG(0);
            Glob::led2.setDimB(statusBlink);
          } else {
            Glob::led2.setDimG(statusBlink);
            Glob::led2.setDimB(0);
          }
          statusBlink = !statusBlink;
          lastCallTemp = millis();
          getCoreTemp(); // read raspi's core temperature
        }

        // do this every 5000ms (every 1 seconds)
        if (millis() - lastCall > 10000) {
          lastCall = millis();
          internetConnected = isInternetConnected();
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
              {
                std::lock_guard<std::mutex> lockMotorTiles(Glob::motors.mut);
                Glob::motorBoard.muteAll();
              }
              cameraDetached = true;
              Glob::a_restartUnfoldingFlag = true;
            }
          }
          if (cameraDetached == false) {   // if camera should be there...
            if (timeSinceNewData > 4000) { // but there is no frame for 4s
              cout << "________________________________________________" << endl
                   << endl;
              cout << "Library Crashed! Reinitialize Camera and Listener. "
                      "last "
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
              {
                std::lock_guard<std::mutex> lockMotorTiles(Glob::motors.mut);
                Glob::motorBoard.muteAll();
              }
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
  {
    std::lock_guard<std::mutex> lockMotorTiles(Glob::motors.mut);
    Glob::motorBoard.muteAll();
  }
  Glob::led1.off();
  Glob::led2.off();
  return 0;
}

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
    // counter for setting off the motors by position. start with
    int offThresh = 12;
    // start with hight counter to mute fast
    int offThreshCounter = offThresh - 1;
    int onThreshCounter = 0;

    while (1) {
      {
        std::unique_lock<std::mutex> svCondLock(Glob::notifySend.mut);
        Glob::notifySend.cond.wait(svCondLock,
                                   [] { return Glob::notifySend.flag; });
      }
      // IF in regular mode
      if (!Glob::modes.a_testMode) {
        {
          std::lock_guard<std::mutex> lockMotorTiles(Glob::motors.mut);
          Glob::motorBoard.sendValuesToGlove(Glob::motors.tiles, 9);
        }
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
      } // IF in test mode
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
      Glob::logger.imuLog.reset();
      Glob::logger.imuLog.store("start");

      // Check if glove position is "active"
      {
        std::lock_guard<std::mutex> lockimu(Glob::imuMux);
        Glob::imu.getPosition();
      }
      if (Glob::modes.a_doLogPrint) {
        std::lock_guard<std::mutex> lockimu(Glob::imuMux);
        Glob::imu.printPosition();
      }
      bool offThreshEx;
      bool onThreshEx;
      {
        std::lock_guard<std::mutex> lockimu(Glob::imuMux);
        offThreshEx = Glob::imu.offThreshExceeded();
        onThreshEx = Glob::imu.onThreshExceeded();
      }
      bool nowActive;
      // if it was "active" before, but not any more:
      if (Glob::modes.a_isInActivePos == true) {
        nowActive = true;
        if (offThreshEx) {
          // if threshold is exceeded: incerement counter
          offThreshCounter++;
        } else {
          // reset when back in prev position
          offThreshCounter = 0;
        }
        if (offThreshCounter > offThresh) {
          Glob::modes.a_muted = true;
          {
            std::lock_guard<std::mutex> lockMotorTiles(Glob::motors.mut);
            Glob::motorBoard.runOnOffPattern(50, 40, 1);
            Glob::motorBoard.runOnOffPattern(190, 0, 1);
            Glob::motorBoard.muteAll();
          }
          nowActive = false;
        }
      } else {
        nowActive = false;
        // if glove is back in use position
        if (onThreshEx) {
          // if threshold is exceeded: incerement counter
          onThreshCounter++;
        } else {
          // reset when back in prev position
          onThreshCounter = 0;
        }
        if (onThreshCounter > 3) {
          {
            std::lock_guard<std::mutex> lockMotorTiles(Glob::motors.mut);
            Glob::motorBoard.runOnOffPattern(50, 40, 2);
          }
          Glob::modes.a_muted = false;
          nowActive = true;
        }
      }
      // Save whether glove is in active position or not
      Glob::modes.a_isInActivePos = nowActive;

      Glob::logger.imuLog.store("end");
      Glob::logger.imuLog.printAll("TIME FOR IMU", "us", "ms");
      Glob::logger.imuLog.reset();
    }
  }
  std::thread runSendDepthDataThread() {
    return std::thread([=] { runSendDepthData(); });
  }
};

//----------------------------------------------------------------------
// MAIN LOOP
//----------------------------------------------------------------------
int main(int ac, char *av[]) {
  // catch cmd line options
  try {
    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
        "log", "enable general log functions – currently "
               "no effect")("printLogs", "print log messages in "
                                         "console")(
        "version", "print verson info "
                   "and exit")("mode", po::value<unsigned int>(),
                               "set "
                               "pico "
                               "flexx "
                               "camera"
                               " mode "
                               "(int "
                               "from "
                               "0:5)")("id", po::value<unsigned int>(),
                                       "set identifier for udp "
                                       "messages");

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    // print help msg
    if (vm.count("help")) {
      cout << desc << "\n";
      return 0;
    }

    // activate general logging
    if (vm.count("log")) {
      // don't use this at the moment – dependecies with msSinceEntry()
      // Glob::modes.a_doLog = true;
      cout << "\n\nlog time and errors – currently this option has no effect\n";
    }

    // prints logs to console if enabled
    if (vm.count("printLogs")) {
      Glob::modes.a_doLogPrint = true;
      cout << "\n\nPrinting of logs in command line\n";
    }

    // set camera mode of pico flexx
    if (vm.count("mode")) {
      Glob::modes.a_cameraUseCase = vm["mode"].as<unsigned int>();
      cout << "\n\nPico flexx mode was set to " << vm["mode"].as<unsigned int>()
           << ".\n";
    } else {
      cout << "\n\nPico Flexx mode was not set manually and therefore is 3.\n";
    }

    // always print version flag
    cout << VERSION << std::endl;
    if (vm.count("version")) {
      return 0; // return (flag already printed)
    }

    if (vm.count("id")) {
      Glob::modes.a_identifier = vm["id"].as<unsigned int>();
    }

  } catch (std::exception &e) {
    cerr << "error: " << e.what() << "\n";
    return 1;
  } catch (...) {
    cerr << "Exception of unknown type!\n";
    return 1;
  }

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
