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
#include "camera.hpp"
#include "glove.hpp"
#include "init.hpp"
#include "poti.hpp"
#include "time.h"
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <royale/IEvent.hpp>
#include <signal.h>
#include <string>

using std::cerr;
using std::cout;
using std::endl;

//----------------------------------------------------------------------
// SETTINGS
//----------------------------------------------------------------------
// any visual output on the raspberry? (or just udp and terminal output...)
bool gui = 0;

// Does the system use a poti?
bool potiAv = 0;

// Does the system use the old DRV-Breakoutboards or the new detachable DRV-PCB?
bool detachableDRV = 0;

// recording mode available?
bool recordMode = false;

//----------------------------------------------------------------------
// DECLARATIONS AND VARIABLES
//----------------------------------------------------------------------
double coreTempDouble; // Temperature of the Raspi core
int droppedAtBridge;   // How many frames got dropped at Bridge (in libroyale)
int droppedAtFC;       // How many frames got dropped at FC (in libroyale)
int deliveredFrames;   // Number of frames delivered
int tenSecsDrops;      // Number of drops in the last 10 seconds
int fpsFromCam;        // wich royal use case is used? (how many fps?)
bool processingImg;    // Image is currently processed
int currentKey = 0;
int libraryCrashNo;        // counter for the crashes of the library
int longestTimeNoData;     // the longest timespan without new data since start
bool connected;            // camera is currently connected
bool capturing;            // camera is currently capturing
long timeSinceLastNewData; // time passed since last "onNewData"
bool cameraDetached;       // camera got detached
long cameraStartTime;      // timestamp when camera started capturing
bool record = false;       // currently recording?

//----------------------------------------------------------------------
// STUFF FOR UDP CONNECTION
//----------------------------------------------------------------------
using boost::asio::ip::udp;
boost::asio::io_service io_service;
// output socket is on port 53333, could be any port
udp::socket myOutputSocket(io_service, udp::endpoint(udp::v4(), 53333));
udp::endpoint remote_endpoint;
boost::system::error_code ignored_error;
udp::endpoint destination(boost::asio::ip::address_v4::broadcast(), 53333);

//________________________________________________
void boostInit() {
  myOutputSocket.open(udp::v4(), ignored_error);
  myOutputSocket.set_option(udp::socket::reuse_address(true));
  myOutputSocket.set_option(boost::asio::socket_base::broadcast(true));
}

//________________________________________________
void sendString(std::string thisString, int thisId) {
  myOutputSocket.send_to(
      boost::asio::buffer(std::to_string(thisId) + ":" + thisString),
      destination, 0, ignored_error);
}

//________________________________________________
void sendInt(int thisInt, int thisId) {
  myOutputSocket.send_to(boost::asio::buffer(std::to_string(thisId) + ":" +
                                             std::to_string(thisInt)),
                         destination, 0, ignored_error);
}

//________________________________________________
void sendLong(long thisLong, int thisId) {
  myOutputSocket.send_to(boost::asio::buffer(std::to_string(thisId) + ":" +
                                             std::to_string(thisLong)),
                         destination, 0, ignored_error);
}

//________________________________________________
void sendBool(bool thisBool, int thisId) {
  myOutputSocket.send_to(boost::asio::buffer(std::to_string(thisId) + ":" +
                                             std::to_string(thisBool)),
                         destination, 0, ignored_error);
}

//________________________________________________
void sendDouble(bool thisDouble, int thisId) {
  myOutputSocket.send_to(boost::asio::buffer(std::to_string(thisId) + ":" +
                                             std::to_string(thisDouble)),
                         destination, 0, ignored_error);
}

// Send the Set of Data to the Android App
void udpHandling() {
  sendString(std::to_string(time(0)), 0);
  sendLong(timeSinceLastNewData, 1);
  sendInt(longestTimeNoData, 2);
  sendInt(fps, 3);
  sendInt(globalPotiVal, 4);
  sendDouble(coreTempDouble, 5);
  sendBool(connected, 6);
  sendBool(capturing, 7);
  sendInt(libraryCrashNo, 8);
  sendInt(droppedAtBridge, 9);
  sendInt(droppedAtFC, 10);
  sendInt(tenSecsDrops, 11);
  sendInt(deliveredFrames, 12);
  sendInt(globalCycleTime, 13);
  sendInt(globalPauseTime, 14);
  for (size_t i = 0; i < 9; i++) {
    sendInt(ninePixMatrix[i], i + 15);
  }
}

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
      if (i == 0)
        droppedAtBridge = found;
      if (i == 1)
        droppedAtFC = found;
      if (i == 2)
        deliveredFrames = found;
      i++;
    }
    /* To save from space at the end of string */
    temp = "";
  }
  tenSecsDrops += droppedAtBridge + droppedAtFC;
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
  coreTempDouble = std::stod(coreTemp);
  tempFile.close();
}

//________________________________________________
// When an error occurs(camera gets detached): prevent freezing, but mute all
void endMuted(int dummy) {
  motorsMuted = true;
  muteAll();
  exit(0);
}

//----------------------------------------------------------------------
// MAIN LOOP
//----------------------------------------------------------------------
int main(int argc, char *argv[]) {

  //_____________________INIT CAMERA____________________________________
  // check if the cam is connected before init anything
  while (checkCam() == false) {
    cout << ".";
    cout.flush();
  }

  // Mute the LRAs before ending the program by ctr + c (SIGINT)
  signal(SIGINT, endMuted);

  // initialize boost library
  boostInit();

  // Setup Connection to Digispark Board for Poti-Reads
  if (potiAv)
    initPoti();

  // Just needed if frames should be recorded
  if (recordMode) {
    // Get Time to create filename for video recording
    time_t now = time(0);
    tm *ltm = localtime(&now);
    std::string a = std::to_string(ltm->tm_mon);
    std::string b = std::to_string(ltm->tm_mday);
    std::string c = std::to_string(ltm->tm_hour);
    std::string d = std::to_string(ltm->tm_min);
    std::string e = std::to_string(ltm->tm_sec);
    std::string depfile = "outputs/" + a + "_" + b + "_" + c + "_" + d + "_" +
                          e + "_" + "depth.avi";
    std::string tilefile = "outputs/" + a + "_" + b + "_" + c + "_" + d + "_" +
                           e + "_" + "tiles.avi";
    cv::VideoWriter depVideo(depfile, CV_FOURCC('X', 'V', 'I', 'D'), 10,
                             cv::Size(224, 171));
    cv::VideoWriter tileVideo(tilefile, CV_FOURCC('X', 'V', 'I', 'D'), 10,
                              cv::Size(3, 3));
  }

searchCam:

  // Setup the LRAs on the Glove (I2C Connection, Settings, Calibration, etc.)
  setupGlove();

  // This is the data listener which will receive callbacks.  It's declared
  // before the cameraDevice so that, if this function exits with a 'return'
  // statement while  camera is still capturing, it will still be in scope
  // until the cameraDevice's destructor implicitly de-registers  listener.
  DepthDataListener listener;

  // Event Listener
  EventReporter eventReporter;

  // this represents the main camera device object
  std::unique_ptr<royale::ICameraDevice> cameraDevice;
  uint commandLineUseCase;

  // commandLineUseCase = std::move (arg);
  commandLineUseCase = atoi(argv[1]);
  // the camera manager will query for a connected camera
  {
    royale::CameraManager manager;
    // EventListener registrieren
    // nicht mehr an dieser Stelle:
    // manager.registerEventListener (&eventReporter);

    royale::Vector<royale::String> camlist;
    cout << "Searching for 3D camera" << endl;
    cout << "_______________________" << endl;
    while (camlist.empty()) {
      // if no argument was given try to open the first connected camera
      camlist = manager.getConnectedCameraList();
      cout << ".";
      cout.flush();
      udpHandling(); // send values via udp

      if (!camlist.empty()) {
        cout << endl;
        cout << "Camera detected!" << endl;
        cameraDevice = manager.createCamera(camlist[0]);
      }
    }
    camlist.clear();
  }
  udpHandling(); // send values via udp

  //  camera device is now available, CameraManager can be deallocated here
  if (cameraDevice == nullptr) {
    // no cameraDevice available
    cerr << "Cannot create the camera device" << endl;
    return 1;
  }

  // IMPORTANT: call the initialize method before working with camera device
  auto status = cameraDevice->initialize();
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
  udpHandling(); // send values via udp

  cerr << useCases << endl;
  udpHandling(); // send values via udp

  // choose a use case
  uint selectedUseCaseIdx = 0u;
  if (commandLineUseCase) {
    cerr << "got the argument:" << commandLineUseCase << endl;
    auto useCaseFound = false;
    if (commandLineUseCase >= 0 && commandLineUseCase < useCases.size()) {
      uint8_t fpsUseCases[6] = {0, 10, 15, 25, 35, 45}; // pico flexx specs
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
  udpHandling(); // send values via udp

  // retrieve the lens parameters from Royale
  royale::LensParameters lensParameters;
  status = cameraDevice->getLensParameters(lensParameters);
  if (status != royale::CameraStatus::SUCCESS) {
    cerr << "Can't read out the lens parameters" << endl;
    return 1;
  }

  //ä listener.setLensParameters(lensParameters);

  // register a data listener
  if (cameraDevice->registerDataListener(&listener) !=
      royale::CameraStatus::SUCCESS) {
    cerr << "Error registering data listener" << endl;
    return 1;
  }
  // register a EVENT listener
  cameraDevice->registerEventListener(&eventReporter);

  if (gui) {
    createWindows();
  }
  udpHandling(); // send values via udp

  //_____________________START CAPTURING_________________________________

  // start capture mode
  if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS) {
    cerr << "Error starting the capturing" << endl;
    return 1;
  }

  // Reset some things
  cameraStartTime = millis(); // set timestamp for capturing start
  cameraDetached = false;     // camera is attached and ready
  motorsMuted = false;        // activate the vibration motors
  long counter = 0;
  long lastCallImshow = millis();
  long lastCall = 0;
  long lastCallPoti = millis();

  //_____________________ENDLESS LOOP_________________________________
  while (currentKey != 27) //...until Esc is pressed
  {
    // only do all of this stuff when the camera is attached
    if (cameraDetached == false) {
      royale::String id;
      royale::String name;
      uint16_t maxSensorWidth;
      uint16_t maxSensorHeight;
      bool calib;
      // time passed since LastNewData
      timeSinceLastNewData = millis() - lastNewData;
      // check if it is a new record
      if (longestTimeNoData < timeSinceLastNewData) {
        longestTimeNoData = timeSinceLastNewData;
      }

      // do this every 66ms (15 fps)
      if (millis() - lastCallImshow > 66) {
        lastCallImshow = millis();
        // Get all the data of the royal lib to see if camera is working
        royale::Vector<royale::Pair<royale::String, royale::String>> cameraInfo;
        auto status = cameraDevice->getCameraInfo(cameraInfo);
        status = cameraDevice->getMaxSensorHeight(maxSensorHeight);
        status = cameraDevice->getMaxSensorWidth(maxSensorWidth);
        status = cameraDevice->getCameraName(name);
        status = cameraDevice->getId(id);
        status = cameraDevice->isCalibrated(calib);
        status = cameraDevice->isConnected(connected);
        status = cameraDevice->isCapturing(capturing);

        if (gui) { // only show when gui is active
          // Draw tile image and depth image in windows
          if (newDepthImage == true) {
            newDepthImage = false;
            cv::Mat dep;
            cv::Mat tile;
            dep = passDepFrame();
            tile = passNineFrame();
            cv::cvtColor(dep, dep, cv::COLOR_HSV2RGB, 3);
            cv::flip(dep, dep, -1);
            if (record == true) {
              // depVideo.write(dep);
              // tileVideo.write(tile);
            }
            cv::imshow("depImg8", dep);
            cv::imshow("tileImg8", tile);
            currentKey = cv::waitKey(1);
            processingImg = false;
          }
        }
      }
      // do this every 50ms (20 fps)
      if (millis() - lastCallPoti > 50) {
        lastCallPoti = millis();
        udpHandling(); // send values via udp
        if (potiAv)
          updatePoti();
        if (record == true) {
          printf("___recording!___\n");
        }
        printf("time since last new data: %i ms \n", timeSinceLastNewData);
        printf("No of library crashes: %i times \n", libraryCrashNo);
        printf("longest time with no new data was: %i \n", longestTimeNoData);
        printf("temp.: \t%.1f°C\n", coreTempDouble);
        printf("drops:\t%i | %i\t deliver:\t%i \t drops in last 10sec: %i\n",
               droppedAtBridge, droppedAtFC, deliveredFrames, tenSecsDrops);
        printOutput();
      }

      // do this every 5000ms (every 10 seconds)
      if (millis() - lastCall > 10000) {
        lastCall = millis();
        tenSecsDrops = 0;
        getCoreTemp(); // read raspi's core temperature
        counter++;
        // display some information about the connected camera
        cout << endl;
        cout << "cycle no " << counter << "  --- " << micros() << endl;
        cout << "====================================" << endl;
        cout << "        Camera information" << endl;
        cout << "====================================" << endl;
        cout << "Id:              " << id << endl;
        cout << "Type:            " << name << endl;
        cout << "Width:           " << maxSensorWidth << endl;
        cout << "Height:          " << maxSensorHeight << endl;
        cout << "Calibrated?:     " << calib << endl;
        cout << "Connected?:      " << connected << endl;
        cout << "Capturing?:      " << capturing << endl;
        cerr << "camera info: " << status << endl
             << endl
             << endl
             << endl
             << endl
             << endl
             << endl
             << endl
             << endl
             << endl;
      }

      // start/stop recording, when recording mode is available
      if (recordMode) {
        if (currentKey == 'r') {
          record = true;
        }

        if (currentKey == 's') {
          record = false;
        }
      }
    }

    // RESTART WHEN CAMERA IS UNPLUGGED
    // Ignore first 3secs
    if ((millis() - cameraStartTime) > 3000) {
      // connected but still capturing -> unplugged!
      if (connected == 0 && capturing == 1) {
        if (cameraDetached == false) {
          cout << "________________________________________________" << endl
               << endl;
          cout << "Camera Detached! Reinitialize Camera and Listener" << endl
               << endl;
          cout << "________________________________________________" << endl
               << endl;
          cout << "Searching for 3D camera in loop" << endl;
          // stop writing new values to the LRAs
          motorsMuted = true;
          // mute all LRAs
          muteAll();
          cameraDetached = true;
        }
        if (checkCam() == false) {
          cout << "."; // print dots as a sign for each failed frame
          cout.flush();
        } else {
          goto searchCam; // jump back to the beginning
        }
      }
    }

    if ((millis() - cameraStartTime) > 3000) { // ignore the first 3 secs
      if (cameraDetached == false) {           // if camera should be there...
        if (timeSinceLastNewData > 4000) {     // but there is no frame for 4s
          cout << "________________________________________________" << endl
               << endl;
          cout << "Library Crashed! Reinitialize Camera and Listener. last new "
                  "frame:  "
               << timeSinceLastNewData << endl
               << endl;
          cout << "________________________________________________" << endl
               << endl;
          libraryCrashNo++;
          // stop writing new values to the LRAs
          motorsMuted = true;
          // mute all LRAs
          muteAll();
          // go to the beginning and find camera again
          goto searchCam;
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
  motorsMuted = true;
  // mute all motors
  muteAll();
  return 0;
}
