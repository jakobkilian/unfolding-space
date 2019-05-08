#include "glove.hpp"
#include "camera.hpp"
#include "poti.hpp"
#include "time.h"
#include <royale/IEvent.hpp>
#include <signal.h>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using std::cerr;
using std::cout;
using std::endl;

double coreTempDouble;
int droppedAtBridge;
int droppedAtFC;
int deliveredFrames;
int tenSecsDrops;
int fpsFromCam;        //wich royal use case is used? How many fps does the cam deliver in this case?
bool processingImg;
int currentKey = 0;
bool record=false;
int libraryCrashNo;
int longestTimeNoData;

long cameraStartTime;

//UDP STUFF
using boost::asio::ip::udp;
boost::asio::io_service io_service;
udp::socket myInputSocket(io_service, udp::endpoint(udp::v4(), 52222));
udp::socket myOutputSocket(io_service, udp::endpoint(udp::v4(), 53333));




void udpHandling(){
  //Check if bytes are available
  boost::asio::socket_base::bytes_readable command(true);
  myInputSocket.io_control(command);
  size_t bytes_readable = command.get();

  if (bytes_readable>0){
        cout << "there are bytes in BUFFER" << '\n';
    boost::array<char, 1> recv_buf;
    udp::endpoint remote_endpoint;
    boost::system::error_code error;


    while (bytes_readable>=1){
      myInputSocket.receive_from(boost::asio::buffer(recv_buf),
      remote_endpoint, 0, error);
      cout << "in receive" << '\n';
      boost::asio::socket_base::bytes_readable command(true);
      myInputSocket.io_control(command);
      bytes_readable = command.get();

    }

    cout << "after receive" << '\n';

    if (error && error != boost::asio::error::message_size)
    throw boost::system::system_error(error);
    cout << "after error" << '\n';
    if (recv_buf[0]=='1'){
      cout << "yes it is a one" << '\n';
      std::string message = std::to_string(time(0));
      boost::system::error_code ignored_error;
      myOutputSocket.send_to(boost::asio::buffer(message),
      remote_endpoint, 0, ignored_error);
      myOutputSocket.send_to(boost::asio::buffer(string("1:") + std::to_string(globalPotiVal)),
      remote_endpoint, 0, ignored_error);
      myOutputSocket.send_to(boost::asio::buffer(string("2:") + std::to_string(longestTimeNoData)),
      remote_endpoint, 0, ignored_error);
    }
  }
}





//Royale Event Listener reports dropped frames as string. This functions extracts the number of frames that got lost at Bridge/FC. I believe, that dropped frames cause the instability.
void extractDrops(royale::String str)
{
  using namespace std;
  stringstream ss;

  /* Storing the whole string into string stream */
  ss << str;

  /* Running loop till the end of the stream */
  string temp;
  int found;
  int i=0;
  while (!ss.eof()) {
    /* extracting word by word from stream */
    ss >> temp;
    /* Checking the given word is integer or not */
    if (stringstream(temp) >> found) {
      if (i==0) droppedAtBridge=found;
      if (i==1) droppedAtFC=found;
      if (i==2) deliveredFrames=found;
      i++;
    }
    /* To save from space at the end of string */
    temp = "";
  }
  tenSecsDrops+=droppedAtBridge +droppedAtFC;

}


//Gets called by Royale irregularily
class EventReporter : public royale::IEventListener
{
public:
  virtual ~EventReporter() = default;

  virtual void onEvent (std::unique_ptr<royale::IEvent> &&event) override
  {
    //printf("EventListener  ");
    royale::EventSeverity severity = event->severity();

    switch (severity)
    {
      case royale::EventSeverity::ROYALE_INFO:
      //cerr << "info: " << event->describe() << endl;
      extractDrops(event->describe());
      break;
      case royale::EventSeverity::ROYALE_WARNING:
      //cerr << "warning: " << event->describe() << endl;
      extractDrops(event->describe());
      break;
      case royale::EventSeverity::ROYALE_ERROR:
      cerr << "error: " << event->describe() << endl;
      break;
      case royale::EventSeverity::ROYALE_FATAL:
      cerr << "fatal: " << event->describe() << endl;
      break;
      default:
      //cerr << "waits..." << event->describe() << endl;
      break;
    }
  }
};

//Read out the core temperature and save it in coreTempDouble for printing it to the console
void getCoreTemp() {
  std::string coreTemp;
  std::ifstream tempFile ("/sys/class/thermal/thermal_zone0/temp");
  tempFile >> coreTemp;
  coreTemp=coreTemp.insert(2,1, '.');
  coreTempDouble= std::stod(coreTemp);
  tempFile.close();
}

void endMuted(int dummy){
  stopWritingVals=true;
  muteAll();
  exit(0);
}




//_______________MAIN LOOP________________________________________________________________________________________________________________________________________________
int main(int argc, char *argv[])
{
  //Mute the LRAs before ending the program by ctr + c (SIGINT)
  signal(SIGINT, endMuted);


  //Setup the LRAs on the Glove (I2C Connection, Settings, Calibration, etc.)
  setupGlove();

  //Setup Connection to Digispark Board for Poti-Reads
  initPoti();
  /*
  //Get Time to create filename for video recording
  time_t now = time(0);
  tm *ltm = localtime(&now);
  std::string a= std::to_string(ltm->tm_mon);
  std::string b= std::to_string(ltm->tm_mday);
  std::string c= std::to_string(ltm->tm_hour);
  std::string d= std::to_string(ltm->tm_min);
  std::string e= std::to_string(ltm->tm_sec);
  std::string depfile = "outputs/" + a +"_" + b +"_"+ c+"_" + d +"_"+ e+"_" + "depth.avi" ;
  std::string tilefile = "outputs/" + a +"_" + b +"_"+ c+"_" + d +"_"+ e+"_" + "tiles.avi" ;
  cv::VideoWriter depVideo(depfile,CV_FOURCC('X','V','I','D'),10, cv::Size(224,171));
  cv::VideoWriter tileVideo(tilefile,CV_FOURCC('X','V','I','D'),10, cv::Size(3,3));

  */

  searchCam:

  // This is the data listener which will receive callbacks.  It's declared
  // before the cameraDevice so that, if this function exits with a 'return'
  // statement while the camera is still capturing, it will still be in scope
  // until the cameraDevice's destructor implicitly de-registers the listener.
  DepthDataListener listener;

  //Event Listener
  EventReporter eventReporter;

  // this represents the main camera device object
  std::unique_ptr<royale::ICameraDevice> cameraDevice;
  uint commandLineUseCase;

  //auto arg = std::unique_ptr<royale::String> (new royale::String (argv[1]));
  //commandLineUseCase = std::move (arg);
  commandLineUseCase = atoi( argv[1]);


  // the camera manager will query for a connected camera
  {
    royale::CameraManager manager;
    //EventListener registrieren
    //nicht mehr an dieser Stelle:
    //manager.registerEventListener (&eventReporter);


    royale::Vector<royale::String> camlist;
    cout << "Searching for 3D camera" << endl;
    cout << "_______________________" << endl;
    while (camlist.empty()){

      // if no argument was given try to open the first connected camera
      camlist= manager.getConnectedCameraList();
      cout << ".";
      cout.flush();

      if (!camlist.empty())
      {
        cout << endl;
        cout << "Camera detected!" << endl;
        cameraDevice = manager.createCamera(camlist[0]);
      }
    }
    /*
    if (camlist.empty())
    {
    cerr << "No suitable camera device detected." << endl
    << "Please make sure that a supported camera is plugged in, all drivers are "
    << "installed, and you have proper USB permission" << endl;
    return 1;
  }
  */
  camlist.clear();

}
// the camera device is now available and CameraManager can be deallocated here
if (cameraDevice == nullptr)
{
  // no cameraDevice available
  if (argc > 1)
  {
    cerr << "Could not open " << argv[1] << endl;
    return 1;
  }
  else
  {
    cerr << "Cannot create the camera device" << endl;
    return 1;
  }
}

// IMPORTANT: call the initialize method before working with the camera device
auto status = cameraDevice->initialize();
if (status != royale::CameraStatus::SUCCESS)
{
  cerr << "Cannot initialize the camera device, error string : " << getErrorString(status) << endl;
  return 1;
}

//mein stuff: cameraDevice->setExposureMode(royale::ExposureMode AUTO);

royale::Vector<royale::String> useCases;
auto usecaseStatus = cameraDevice->getUseCases(useCases);

if (usecaseStatus != royale::CameraStatus::SUCCESS || useCases.empty())
{
  cerr << "No use cases are available" << endl;
  cerr << "getUseCases() returned: " << getErrorString(usecaseStatus) << endl;
  return 1;
}

cerr << useCases << endl;

// choose a use case
uint selectedUseCaseIdx = 0u;
if (commandLineUseCase)
{
  cerr << "got the argument:" << commandLineUseCase << endl;
  auto useCaseFound = false;
  if (commandLineUseCase >= 0 && commandLineUseCase < useCases.size())
  {
    selectedUseCaseIdx = commandLineUseCase;
    switch (commandLineUseCase)
    {
      case 0:
      fpsFromCam=5;
      break;
      case 1:
      fpsFromCam=10;
      break;
      case 2:
      fpsFromCam=15;
      break;
      case 3:
      fpsFromCam=25;
      break;
      case 4:
      fpsFromCam=35;
      break;
      case 5:
      fpsFromCam=45;
      break;
    }
    useCaseFound = true;
  }


  if (!useCaseFound)
  {
    cerr << "Error: the chosen use case is not supported by this camera" << endl;
    cerr << "A list of supported use cases is printed by sampleCameraInfo" << endl;
    return 1;
  }
}
else
{
  cerr << "Here: autousecase id" << endl;
  // choose the first use case
  selectedUseCaseIdx = 0;
}

// set an operation mode
if (cameraDevice->setUseCase(useCases.at(selectedUseCaseIdx)) != royale::CameraStatus::SUCCESS)
{
  cerr << "Error setting use case" << endl;
  return 1;
}

// retrieve the lens parameters from Royale
royale::LensParameters lensParameters;
status = cameraDevice->getLensParameters(lensParameters);
if (status != royale::CameraStatus::SUCCESS)
{
  cerr << "Can't read out the lens parameters" << endl;
  return 1;
}

//listener.setLensParameters(lensParameters);

// register a data listener
if (cameraDevice->registerDataListener(&listener) != royale::CameraStatus::SUCCESS)
{
  cerr << "Error registering data listener" << endl;
  return 1;
}
// register a EVENT listener
cameraDevice->registerEventListener (&eventReporter);



// create two windows
cv::namedWindow ("tileImg8", cv::WINDOW_NORMAL);
cv::resizeWindow("tileImg8", 672,513);
cv::moveWindow("tileImg8", 700, 50);
cv::namedWindow ("depImg8", cv::WINDOW_NORMAL);
cv::resizeWindow("depImg8", 672,513);
cv::moveWindow("depImg8", 50, 50);



// start capture mode

if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS)
{
  cerr << "Error starting the capturing" << endl;
  return 1;
}

cameraStartTime=millis();
//active the vibration motors
stopWritingVals=false;


long counter=0;
long lastCallImshow=millis();
long lastCall=0;
long lastCallPoti=millis();
while (currentKey != 27)
{
  royale::String id;
  royale::String name;
  uint16_t maxSensorWidth;
  uint16_t maxSensorHeight;
  bool calib;
  bool connected;
  bool capturing;
  long timeSinceLastNewData= millis()-lastNewData;
  if (longestTimeNoData<timeSinceLastNewData){
    longestTimeNoData=timeSinceLastNewData;
  }



  if (millis()-lastCallImshow> 100) {

    //Get all the data of the royal lib to see if camera is working
    royale::Vector<royale::Pair<royale::String, royale::String>> cameraInfo;
    auto status = cameraDevice->getCameraInfo (cameraInfo);
    status = cameraDevice->getMaxSensorHeight (maxSensorHeight);
    status = cameraDevice->getMaxSensorWidth (maxSensorWidth);
    status = cameraDevice->getCameraName (name);
    status = cameraDevice->getId (id);
    status = cameraDevice->isCalibrated (calib);
    status = cameraDevice->isConnected (connected);
    status = cameraDevice->isCapturing (capturing);

    lastCallImshow=millis();
    if (newDepthImage==true) {
      newDepthImage=false;
      cv::Mat dep;
      cv::Mat tile;
      dep=passDepFrame();
      tile=passNineFrame();
      cv::cvtColor(dep, dep, cv::COLOR_HSV2RGB, 3);
      cv::flip(dep,dep, -1);
      if (record==true) {
        //depVideo.write(dep);
        //tileVideo.write(tile);
      }
      cv::imshow ("depImg8", dep);
      cv::imshow ("tileImg8", tile);
      currentKey=cv::waitKey(1);
      processingImg=false;
    }
  }

  if (millis()-lastCallPoti>100) {
    udpHandling();
    updatePoti();
    if (record==true) {
      printf("___recording!___\n");
    }
    printf("time since last new data: %i ms \n", timeSinceLastNewData);
    printf("No of library crashes: %i times \n", libraryCrashNo);
    printf("longest time with no new data was: %i \n", longestTimeNoData);
    printf("temp.: \t%.1f°C\n", coreTempDouble);
    printf("drops:\t%i | %i\t deliver:\t%i \t drops in last 10sec: %i\n", droppedAtBridge,droppedAtFC, deliveredFrames, tenSecsDrops);
    printOutput();
    lastCallPoti=millis();
  }


  if (millis()-lastCall>5000) {
    tenSecsDrops=0;
    getCoreTemp();
    counter++;
    // display some information about the connected camera

    cout << endl;
    cout << "cycle no "<< counter << "  --- " << micros() << endl;
    cout << "====================================" << endl;
    cout << "        Camera information"           << endl;
    cout << "====================================" << endl;
    cout << "Id:              " << id << endl;
    cout << "Type:            " << name << endl;
    cout << "Width:           " << maxSensorWidth << endl;
    cout << "Height:          " << maxSensorHeight << endl;
    cout << "Calibrated?:     " << calib << endl;
    cout << "Connected?:      " << connected << endl;
    cout << "Capturing?:      " << capturing << endl;
    cerr << "camera info: " << status << endl<< endl<< endl<< endl<< endl<< endl<< endl<< endl<< endl<< endl;
    lastCall=millis();
  }
  if (currentKey == 'r')
  {
    record=true;
  }

  if (currentKey == 's')
  {
    record=false;

  }

  //RESTART WHEN CAMERA IS UNPLUGGED
  //Check how long camera is capturing - in the beginning it needs some time to be recognized -> 1000ms
  if((millis()-cameraStartTime)>1000){
    //If it says that it is not connected but still capturing, it should be unplugged:
    if (connected==0 && capturing==1)
    {
      cout << "________________________________________________"<< endl<< endl;
      cout << "Camera Detached! Reinitialize Camera and Listener"<< endl<< endl;
      cout << "________________________________________________"<< endl<< endl;
      //stop writing new values to the LRAs
      stopWritingVals=true;
      //mute all LRAs
      muteAll();
      //go to the beginning and find camera again
      goto searchCam;
    }
    if (timeSinceLastNewData>4000){
      cout << "________________________________________________"<< endl<< endl;
      cout << "Library Crashed! Reinitialize Camera and Listener. last new frame:  "<<timeSinceLastNewData<< endl<< endl;
      cout << "________________________________________________"<< endl<< endl;
      libraryCrashNo++;
      //stop writing new values to the LRAs
      stopWritingVals=true;
      //mute all LRAs
      muteAll();
      //go to the beginning and find camera again
      goto searchCam;
    }
  }
}

// stop capture mode
if (cameraDevice->stopCapture() != royale::CameraStatus::SUCCESS)
{
  cerr << "Error stopping the capturing" << endl;
  return 1;
}
exitprog:
stopWritingVals=true;
//Alle Motoren ausschalten
muteAll();

return 0;
}
