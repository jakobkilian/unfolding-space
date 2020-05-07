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
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <chrono>
#include <ctime>
#include <iostream>
#include <royale/IEvent.hpp>
#include <sstream>
#include <string>
#include <thread>

#include "camera.hpp"
#include "glove.hpp"
#include "init.hpp"
#include "poti.hpp"
#include "time.h"

using boost::asio::ip::udp;
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
long timeSinceLastNewData;  // time passed since last "onNewData"
double coreTempDouble;      // Temperature of the Raspi core
int droppedAtBridge;    // How many frames got dropped at Bridge (in libroyale)
int droppedAtFC;        // How many frames got dropped at FC (in libroyale)
int deliveredFrames;    // Number of frames delivered
int tenSecsDrops;       // Number of drops in the last 10 seconds
int fpsFromCam;         // wich royal use case is used? (how many fps?)
bool processingImg;     // Image is currently processed
int currentKey = 0;     //
int libraryCrashNo;     // counter for the crashes of the library
int longestTimeNoData;  // the longest timespan without new data since start
bool connected;         // camera is currently connected
bool capturing;         // camera is currently capturing
bool cameraDetached;    // camera got detached
long cameraStartTime;   // timestamp when camera started capturing
bool record = false;    // currently recording?

// Motor test stuff
bool testMotors;
uint8_t motorTestMatrix[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

//----------------------------------------------------------------------
// UDP CONNECTION
//----------------------------------------------------------------------

// Helper
template <typename T>
std::string add(unsigned char id, T const &value) {
  std::ostringstream oss;
  oss << id << ":" << std::to_string(value) << "|";
  std::string s = oss.str();
  // printf(id);
  return s;
}

// Send the Set of Data to the Android App
std::string packValStr(bool sendImg, int imgSize) {
  std::string tmpStr;
  // Add the values to the string. TODO: slim everything a bit down? Does string
  // cost time?
  tmpStr += add(0, frameCounter);
  tmpStr += add(1, timeSinceLastNewData);
  tmpStr += add(2, longestTimeNoData);
  tmpStr += add(3, (int)fps);
  tmpStr += add(4, globalPotiVal);
  tmpStr += add(5, coreTempDouble);
  tmpStr += add(6, connected);
  tmpStr += add(7, capturing);
  tmpStr += add(8, libraryCrashNo);
  tmpStr += add(9, droppedAtBridge);
  tmpStr += add(10, droppedAtFC);
  tmpStr += add(11, tenSecsDrops);
  tmpStr += add(12, deliveredFrames);
  tmpStr += add(13, globalCycleTime);
  tmpStr += add(14, globalPauseTime);
  tmpStr += add(15, motorsMuted);
  tmpStr += add(16, testMotors);

  for (size_t i = 0; i < 9; i++) {
    if (!testMotors) {
      tmpStr += add(i + 17, ninePixMatrix[i]);
    } else {
      tmpStr += add(i + 17, motorTestMatrix[i]);
    }
  }
  tmpStr += add(26, millis());
  // Send detailed picture if wanted
  if (sendImg) {
    tmpStr += "#|";
    cv::Mat dep;
    dep = passUdpFrame(imgSize);
    cv::cvtColor(dep, dep, cv::COLOR_HSV2RGB, 3);
    cv::flip(dep, dep, -1);
    for (int i = 0; i < dep.rows; i++) {
      for (int j = 0; j < dep.cols; j++) {
        unsigned char t = dep.at<cv::Vec3b>(i, j)[0];
        tmpStr += t;
      }
    }
  }
  return tmpStr;
}

// UDP Server Class
class udp_brodcasting {
 public:
  udp_brodcasting(boost::asio::io_service &io_service)
      : broad_socket_(io_service, udp::endpoint(udp::v4(), 9007)) {
    broad_socket_.open(udp::v4(), ignored_error);
    broad_socket_.set_option(udp::socket::reuse_address(true));
    broad_socket_.set_option(boost::asio::socket_base::broadcast(true));
    broad_endpoint_ =
        udp::endpoint(boost::asio::ip::address_v4::broadcast(), 9008);
  }
  udp::socket broad_socket_;
  udp::endpoint broad_endpoint_;
  boost::system::error_code ignored_error;

 public:
  void sendBroadcast() {
    if (!ignored_error) {
      broad_socket_.send_to(boost::asio::buffer("Unfolding 1"), broad_endpoint_,
                            0, ignored_error);
    } else {
      broad_socket_.send_to(boost::asio::buffer("Unfolding 1"), broad_endpoint_,
                            0, ignored_error);
      cout << "Broadcast ERROR: " << ignored_error << endl;
    }
  }
};

// UDP Server Class
class udp_server {
 public:
  udp_server(boost::asio::io_service &io_service)
      : socket_(io_service, udp::endpoint(udp::v4(), 9009)) {
    start_receive();
  }
  udp::socket socket_;
  udp::endpoint remote_endpoint_;
  boost::array<char, 4> recv_buffer_;

 private:
  void start_receive() {
    // Look out for calls on port 9009
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), remote_endpoint_,
        boost::bind(&udp_server::handle_receive, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  }

  void handle_receive(const boost::system::error_code &error,
                      std::size_t /*bytes_transferred*/) {
    bool sendImg = false;
    int imgSize = 0;
    int incSize = std::find(recv_buffer_.begin(), recv_buffer_.end(), '\0') -
                  recv_buffer_.begin();
    if (incSize > 0) {
      // printf("not empty\n");
      auto incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'i');
      if (incoming != recv_buffer_.end()) {
        int tmp = (*std::next(incoming, 1) - 48);
        sendImg = true;
        imgSize = tmp;
      }
      incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'm');
      if (incoming != recv_buffer_.end()) {
        motorsMuted = !motorsMuted;
        testMotors = false;
        muteAll();
      }
      incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 't');
      if (incoming != recv_buffer_.end()) {
        testMotors = !testMotors;
        motorsMuted = testMotors;
        muteAll();
      }

      incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'z');
      if (incoming != recv_buffer_.end()) {
        int tmp = (*std::next(incoming, 1) - 48);
        motorTestMatrix[tmp] = motorTestMatrix[tmp] == 0 ? 254 : 0;
        // motorTestMatrix[tmp] = abs(motorTestMatrix[tmp] - 255);
      }

      incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'c');
      if (incoming != recv_buffer_.end()) {
        calibRunning = true;
        muteAll();
        motorsMuted = true;
        doCalibration();
        calibRunning = false;
      }
    }
    if (!error || error == boost::asio::error::message_size) {
      // send the ready packed string with all the values from packValStr
      boost::shared_ptr<std::string> message(
          new std::string(packValStr(sendImg, imgSize)));
      socket_.async_send_to(
          boost::asio::buffer(*message), remote_endpoint_,
          boost::bind(&udp_server::handle_send, this, message,
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));
      // start listening again
      start_receive();
    }
  }

  void handle_send(boost::shared_ptr<std::string> /*message*/,
                   const boost::system::error_code & /*error*/,
                   std::size_t /*bytes_transferred*/) {}
};

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
      if (i == 0) droppedAtBridge = found;
      if (i == 1) droppedAtFC = found;
      if (i == 2) deliveredFrames = found;
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
// UNFOLDING - This is the main part, now in a seperate thread
//----------------------------------------------------------------------

int unfolding() {
  //_____________________INIT CAMERA____________________________________
  // check if the cam is connected before init anything
  while (checkCam() == false) {
    cout << ".";
    cout.flush();
  }

  // Mute the LRAs before ending the program by ctr + c (SIGINT)
  signal(SIGINT, endMuted);

  // initialize boost library
  // boostInit();

  // Setup Connection to Digispark Board for Poti-Reads
  if (potiAv) initPoti();

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
    cv::VideoWriter depVideo(depfile,
                             cv::VideoWriter::fourcc('X', 'V', 'I', 'D'), 10,
                             cv::Size(224, 171));
    cv::VideoWriter tileVideo(tilefile,
                              cv::VideoWriter::fourcc('X', 'V', 'I', 'D'), 10,
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
  commandLineUseCase = 3;
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
      if (!camlist.empty()) {
        cout << endl;
        cout << "Camera detected!" << endl;
        cameraDevice = manager.createCamera(camlist[0]);
      }
    }
    camlist.clear();
  }

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
  // register a EVENT listener
  cameraDevice->registerEventListener(&eventReporter);

  if (gui) {
    createWindows();
  }
  //_____________________START CAPTURING_________________________________
  // start capture mode
  if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS) {
    cerr << "Error starting the capturing" << endl;
    return 1;
  }

  // Reset some things
  cameraStartTime = millis();  // set timestamp for capturing start
  cameraDetached = false;      // camera is attached and ready
  motorsMuted = false;         // activate the vibration motors
  long lastCallImshow = millis();
  long lastCall = 0;
  long lastCallPoti = millis();

  //_____________________ENDLESS LOOP_________________________________
  while (currentKey != 27)  //...until Esc is pressed
  {
    if (!calibRunning) {
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
          status = cameraDevice->isCalibrated(calib);
          status = cameraDevice->isConnected(connected);
          status = cameraDevice->isCapturing(capturing);

          if (gui) {  // only show when gui is active
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
        if (millis() - lastCallPoti > 100) {
          lastCallPoti = millis();
          if (testMotors) {
            writeValues(9, &(motorTestMatrix[0]));
          }

          if (potiAv) updatePoti();
          if (record == true) {
            printf("___recording!___\n");
          } /*
           printf("time since last new data: %i ms \n", timeSinceLastNewData);
           printf("No of library crashes: %i times \n", libraryCrashNo);
           printf("longest time with no new data was: %i \n",
           longestTimeNoData); printf("temp.: \t%.1f°C\n", coreTempDouble);
           printf("drops:\t%i | %i\t deliver:\t%i \t drops in last 10sec: %i\n",
                  droppedAtBridge, droppedAtFC, deliveredFrames, tenSecsDrops);
           printOutput();
           */
        }

        // do this every 5000ms (every 1 seconds)
        if (millis() - lastCall > 1000) {
          lastCall = millis();
          tenSecsDrops = 0;
          getCoreTemp();  // read raspi's core temperature
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
            cout << ".";  // print dots as a sign for each failed frame
            cout.flush();
          } else {
            goto searchCam;  // jump back to the beginning
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

// TODO: maybe this is not the best way to handle this and exit (!) from the
// program
class mainThreadWrapper {
 public:
  boost::asio::io_service io_service;
  boost::asio::io_service io_service2;
  void runUdp() {
    udp_server server(io_service);
    io_service.run();
  }
  void runBroad() {
    udp_brodcasting server2(io_service2);
    while (1) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      server2.sendBroadcast();
    }
  }
  void runUnfolding() {
    int unfReturn = unfolding();
    printf("Unfolding Thread has been exited with a #%i \n", unfReturn);
    printf("Stopping the UDP server now... \n");
    io_service.stop();
  }
  std::thread runUdpThread() {
    return std::thread([=] { runUdp(); });
  }
  std::thread runUnfoldingThread() {
    return std::thread([=] { runUnfolding(); });
  }
  std::thread runUdpBroadcasting() {
    return std::thread([=] { runBroad(); });
  }
};

//----------------------------------------------------------------------
// MAIN LOOP
//----------------------------------------------------------------------
int main(int argc, char *argv[]) {
  printf("Starting Unfolding Space \n");
  mainThreadWrapper *w = new mainThreadWrapper();
  std::thread udpTh = w->runUdpThread();
  std::thread unfTh = w->runUnfoldingThread();
  std::thread udpBroad = w->runUdpBroadcasting();
  udpTh.join();
  unfTh.join();
  udpBroad.join();
  return 0;
}
