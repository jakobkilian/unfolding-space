#include "glove.hpp"
#include "camera.hpp"
#include "poti.hpp"
#include "init.hpp"
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
bool connected;
bool capturing;
long timeSinceLastNewData;
bool cameraDetached;

long cameraStartTime;

//Does the system use a poti?
bool potiAv=0;

//Does the system use the old and DRV-Breakoutboards or the new detachable DRV-PCB
bool detachableDRV=0;

//any visual output?
bool gui=0;



class udp_server
{
public:
  udp_server(boost::asio::io_service& io_service)
    : socket_(io_service, udp::endpoint(udp::v4(), 52222))
  {
    start_receive();
  }

private:
  void start_receive()
  {
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), remote_endpoint_,
        boost::bind(&udp_server::handle_receive, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void handle_receive(const boost::system::error_code& error,
      std::size_t /*bytes_transferred*/)
  {
    if (!error || error == boost::asio::error::message_size)
    {
      boost::shared_ptr<std::string> message(
          new std::string(make_daytime_string()));

      socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
          boost::bind(&udp_server::handle_send, this, message,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));

      start_receive();
    }
  }

  void handle_send(boost::shared_ptr<std::string> /*message*/,
      const boost::system::error_code& /*error*/,
      std::size_t /*bytes_transferred*/)
  {
  }

  udp::socket socket_;
  udp::endpoint remote_endpoint_;
  boost::array<char, 1> recv_buffer_;
};



//UDP STUFF
using boost::asio::ip::udp;
boost::asio::io_service io_service;
udp::socket myOutputSocket(io_service, udp::endpoint(udp::v4(), 53333));
udp::endpoint remote_endpoint;
boost::system::error_code ignored_error;
udp::endpoint destination(boost::asio::ip::address_v4::broadcast(), 53333);

  void boostInit(){
    myOutputSocket.open(udp::v4(),ignored_error);
    myOutputSocket.set_option(udp::socket::reuse_address(true));
    myOutputSocket.set_option(boost::asio::socket_base::broadcast(true));

  }
  void sendString(std::string thisString, int thisId){
    myOutputSocket.send_to(boost::asio::buffer(std::to_string(thisId)+":" + thisString),destination, 0, ignored_error);
  }
  void sendInt(int thisInt, int thisId){
    myOutputSocket.send_to(boost::asio::buffer(std::to_string(thisId)+":" + std::to_string(thisInt)),destination, 0, ignored_error);
  }
  void sendLong(long thisLong, int thisId){
    myOutputSocket.send_to(boost::asio::buffer(std::to_string(thisId)+":" + std::to_string(thisLong)),destination, 0, ignored_error);
  }
  void sendBool(bool thisBool, int thisId){
    myOutputSocket.send_to(boost::asio::buffer(std::to_string(thisId)+":" + std::to_string(thisBool)),destination, 0, ignored_error);
  }
  void sendDouble(bool thisDouble, int thisId){
    myOutputSocket.send_to(boost::asio::buffer(std::to_string(thisId)+":" + std::to_string(thisDouble)),destination, 0, ignored_error);
  }

  void udpHandling(){
    sendString(std::to_string(time(0)),0);
    sendLong(timeSinceLastNewData,1);
    sendInt(longestTimeNoData,2);
    sendInt(fps,3);
    sendInt(globalPotiVal,4);
    sendDouble(coreTempDouble,5);
    sendBool(connected,6);
    sendBool(capturing,7);
    sendInt(libraryCrashNo,8);
    sendInt(droppedAtBridge,9);
    sendInt(droppedAtFC,10);
    sendInt(tenSecsDrops,11);
    sendInt(deliveredFrames,12);
    sendInt(globalCycleTime,13);
    sendInt(globalPauseTime,14);
    for (size_t i = 0; i < 9; i++) {
      sendInt(ninePixMatrix[i],i+15);}
    }

    //Royale Event Listener reports dropped frames as string. This functions extracts the number of frames that got lost at Bridge/FC. I believe, that dropped frames causes instability – PMDtec more or less confirmed this
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

    //When an error occurs or the camera gets detached: mute all vibrations insted showing last image
    void endMuted(int dummy){
      stopWritingVals=true;
      muteAll();
      exit(0);
    }




    //_______________MAIN LOOP________________________________________________________________________________________________________________________________________________
    int main(int argc, char *argv[])
    {

      try
{
  boost::asio::io_service io_service;
  udp_server server(io_service);
  io_service.run();
}
      //check if the cam is connected before init anything
      while (checkCam()==false){
        cout << "." ;
        cout.flush();
      }

      //Mute the LRAs before ending the program by ctr + c (SIGINT)
      signal(SIGINT, endMuted);

      boostInit();


      //Setup Connection to Digispark Board for Poti-Reads
      if (potiAv)
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

      //Setup the LRAs on the Glove (I2C Connection, Settings, Calibration, etc.)
      setupGlove();

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
          udpHandling();
          if (!camlist.empty())
          {
            cout << endl;
            cout << "Camera detected!" << endl;
            cameraDevice = manager.createCamera(camlist[0]);
          }
        }
        camlist.clear();

      }
      udpHandling();

      // the camera device is now available and CameraManager can be deallocated here
      if (cameraDevice == nullptr)
      {
        // no cameraDevice available
          cerr << "Cannot create the camera device" << endl;
          return 1;
      }

      // IMPORTANT: call the initialize method before working with the camera device
      auto status = cameraDevice->initialize();
      if (status != royale::CameraStatus::SUCCESS)
      {
        cerr << "Cannot initialize the camera device, error string : " << getErrorString(status) << endl;
        return 1;
      }

      royale::Vector<royale::String> useCases;
      auto usecaseStatus = cameraDevice->getUseCases(useCases);

      if (usecaseStatus != royale::CameraStatus::SUCCESS || useCases.empty())
      {
        cerr << "No use cases are available" << endl;
        cerr << "getUseCases() returned: " << getErrorString(usecaseStatus) << endl;
        return 1;
      }
      udpHandling();

      cerr << useCases << endl;
      udpHandling();

      // choose a use case
      uint selectedUseCaseIdx = 0u;
      if (commandLineUseCase)
      {
        cerr << "got the argument:" << commandLineUseCase << endl;
        auto useCaseFound = false;
        if (commandLineUseCase >= 0 && commandLineUseCase < useCases.size())
        {
          uint8_t fpsUseCases[6] = {0,10,15,25,35,45}; //fix values coming from the pico flexx
          selectedUseCaseIdx = commandLineUseCase;
          fpsFromCam=fpsUseCases[commandLineUseCase];
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
      udpHandling();

      // retrieve the lens parameters from Royale
      royale::LensParameters lensParameters;
      status = cameraDevice->getLensParameters(lensParameters);
      if (status != royale::CameraStatus::SUCCESS)
      {
        cerr << "Can't read out the lens parameters" << endl;
        return 1;
      }

      //ä listener.setLensParameters(lensParameters);

      // register a data listener
      if (cameraDevice->registerDataListener(&listener) != royale::CameraStatus::SUCCESS)
      {
        cerr << "Error registering data listener" << endl;
        return 1;
      }
      // register a EVENT listener
      cameraDevice->registerEventListener (&eventReporter);


      if (gui){
        createWindows();}


        udpHandling();

        // start capture mode
        if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS)
        {
          cerr << "Error starting the capturing" << endl;
          return 1;
        }

        cameraStartTime=millis();
        cameraDetached=false;
        //active the vibration motors
        stopWritingVals=false;


        long counter=0;
        long lastCallImshow=millis();
        long lastCall=0;
        long lastCallPoti=millis();
        while (currentKey != 27)
        {
          catch (std::exception& e)
{
  std::cerr << e.what() << std::endl;
}
          //only do all of this stuff when the camera is attached
          if (cameraDetached==false){
            royale::String id;
            royale::String name;
            uint16_t maxSensorWidth;
            uint16_t maxSensorHeight;
            bool calib;
            timeSinceLastNewData= millis()-lastNewData;
            if (longestTimeNoData<timeSinceLastNewData){
              longestTimeNoData=timeSinceLastNewData;
            }



            if (millis()-lastCallImshow> 66) {

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
              if (gui){
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
            }

            if (millis()-lastCallPoti>50) {
              udpHandling();
              if (potiAv)
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
          }
          //RESTART WHEN CAMERA IS UNPLUGGED
          //Check how long camera is capturing - in the beginning it needs some time to be recognized -> 1000ms
          if((millis()-cameraStartTime)>3000){
            //If it says that it is not connected but still capturing, it should be unplugged:
            if (connected==0 && capturing==1)
            {
              if (cameraDetached==false){
                cout << "________________________________________________"<< endl<< endl;
                cout << "Camera Detached! Reinitialize Camera and Listener"<< endl<< endl;
                cout << "________________________________________________"<< endl<< endl;
                cout << "Searching for 3D camera in loop" << endl;
                //stop writing new values to the LRAs
                stopWritingVals=true;
                //mute all LRAs
                muteAll();
                cameraDetached=true;
              }
              if (checkCam()==false){
                cout << "." ;
                cout.flush();
              }
              else{
                goto searchCam;
              }
            }

          }

          if((millis()-cameraStartTime)>3000){
            if (cameraDetached==false){
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
              }}
            }

          }
          //__________ END OF KEY-LOOP



          // stop capture mode
          if (cameraDevice->stopCapture() != royale::CameraStatus::SUCCESS)
          {
            cerr << "Error stopping the capturing" << endl;
            return 1;
          }
          stopWritingVals=true;
          //Alle Motoren ausschalten
          muteAll();
          return 0;
        }
