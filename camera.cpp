#include "camera.hpp"
#include "glove.hpp"
#include "poti.hpp"
#include "init.hpp"
#include <sample_utils/EventReporter.hpp>
#include <array>

using std::cerr;
using std::cout;
using std::endl;

using namespace royale;

//when scanning... : the min number of pixels, an object must have (smaller objects might be noise
int minObjSizeThresh = 90;
//This is the Main Distance Threshold in meters: bigger values will be irgnored. Value can be changed by poti rotation
float distThresh = 1.5;

int frameCounter;
int loop;
int fps;
std::array<uint8_t,9> ninePixMatrix;
int globalPotiVal;

int cycleTime;
int lastPrintCurTime;
cv::Mat depImg;
cv::Mat depImgMod;
cv::Mat tileImg;

bool newDepthImage;
bool stopWritingVals=false;
long lastNewData=millis();

int globalCycleTime;
int globalPauseTime;

int width;
int height;

std::mutex depMutex;
std::mutex tileMutex;

//standard size of the Windows
cv::Size_<int> myWindowSize = cv::Size(112, 85);

int millisFirstFrame;
bool first=false;

//
int myHueChange(float oldValue, float changeValue)
{
  int oldInt = static_cast<int>(oldValue);
  int changeInt = static_cast<int>(changeValue);
  return (oldInt + changeInt) % 360;
}

//How long was this step?
void printCurTime(const std::string& msg)
{
  //cout << millis()-lastPrintCurTime <<  " \t" << msg << endl;
  cycleTime+=millis()-lastPrintCurTime;
  lastPrintCurTime=millis();
}

//How long was this cycle?
void getCycleDur()
{
  cycleTime+=millis()-lastPrintCurTime;
  globalCycleTime=cycleTime;
  //cout <<  " _________________________ Cycle Time:" << cycleTime <<  " ______________________________" << endl;
  cycleTime=0;
  lastPrintCurTime=millis();
}

//How long was the pause since last onNewData?
void getPauseDur()
{
  cycleTime+=millis()-lastPrintCurTime;
  globalPauseTime=cycleTime;
  //cout <<  "Pause Time: " << cycleTime <<  "" << endl;
  cycleTime=0;
  lastPrintCurTime=millis();
}
/*
void DepthDataListener::setLensParameters(const LensParameters &lensParameters)
{
// Construct the camera matrix
// (fx   0    cx)
// (0    fy   cy)
// (0    0    1 )
cameraMatrix = (cv::Mat1d(3, 3) << lensParameters.focalLength.first, 0, lensParameters.principalPoint.first,
0, lensParameters.focalLength.second, lensParameters.principalPoint.second,
0, 0, 1);

// Construct the distortion coefficients
// k1 k2 p1 p2 k3
distortionCoefficients = (cv::Mat1d(1, 5) << lensParameters.distortionRadial[0],
lensParameters.distortionRadial[1],
lensParameters.distortionTangential.first,
lensParameters.distortionTangential.second,
lensParameters.distortionRadial[2]);
}
*/


//This function gets called everytime there is a new depth frame from the Pico Flexx
void DepthDataListener::onNewData(const DepthData *data)
{
  int histo[9][256];                      //historgram, needed to find closest obj and creat tiles
  lastNewData=millis();                   //timestamp when new frame arrives (to check if libroyale didn't crash)
  getPauseDur();                          //calcucalte how long the last pause (waiting for onNewData) has been
  if (potiAv){
  distThresh=globalPotiVal/100/3;         //calculate current distThresh from potiVal
}

//Save time of first arriving frame
  if(first!=true) {
    millisFirstFrame=millis();
    first=true;
  }

//check size of incoming data
  width = data->width;                //get width from depth image
  height = data->height;              //get height from depth image
  int tileWidth = width / 3 + 1;      //respectiveley width of one tile
  int tileHeight = height / 3 + 1;    //respectiveley height of one tile

//need this if-true for the mutex
if(true){
  std::lock_guard<std::mutex> lock(depMutex);           //Do not read the image while it gets written
  depImg.create (cv::Size (width, height), CV_8UC3);    //images that get filled later
  tileImg.create (cv::Size (3, 3), CV_8UC1);            //images that get filled later
  bzero(histo, sizeof(int) * 9 * 256);                  //empty histogram array

//READING DEPTH IMAGE pixel by pixel
  for (int y = 0; y < height; y++)
  {
    uint8_t *depImgPtr = depImg.ptr<uint8_t>(y);
    for (int x = 0; x < width; x++)
    {
      auto curPoint = data->points.at(y * width + x);         //save currently observed pixel in curPoint
      bool valid=curPoint.depthConfidence > 10;               //check its validity (if bigger than 10 -> valid)
      int tileIdx = (x / tileWidth) + 3 * (y / tileHeight);   //switch to the respective tile

      //WRITE VALID PIXELS in DepImg and histo
      if (valid)
      {
        // if the point is valid, map the pixel from 3D world
        // coordinates to a 2D plane (this will distort the image)
        uint8_t depth = (uint8_t)adjustDepthValue(curPoint.z, distThresh);      //Depth Value in relation to distThresh
        histo[tileIdx][depth]++;                                                //write pixel in historgram
        depImgPtr[x * 3] = adjustDepthValueForImage(curPoint.z, distThresh);    //ä Hue value from 0-180
        depImgPtr[x * 3 + 1] = 0;                                               //ä Saturation is always the same
        depImgPtr[x * 3 + 2] = adjustDepthValue(curPoint.z, distThresh);        //ä Brightness is always the same
      }

      //ä if pixel is out of range but valid –> make it white / invisible
      if (adjustDepthValue(curPoint.z, distThresh)==255)
      {
        depImgPtr[x * 3] = 255;
        depImgPtr[x * 3 + 1] = 0;
        depImgPtr[x * 3 + 2] = 255;
      }

      //ä If pixel is not valid –> make it grey
      if (!valid)
      {
        depImgPtr[x * 3] = 7;
        depImgPtr[x * 3 + 1] = 10;
        depImgPtr[x * 3 + 2] = 245;
        histo[tileIdx][255]++;                  //ä add a pixel in inivite distance

      }
    }
  }
}
newDepthImage=true;                             //New Depth Image is there

if(true){                                       //need this if-true for the mutex
  std::lock_guard<std::mutex> lock(tileMutex);  //Do not read the image while it gets written
  //FIND CLOSEST object per tile
  for (int tileIdx = 0; tileIdx < 9; tileIdx++)
  {
    int sum = 0;
    int val;
    int offset=17;          //exclude the first 17cm because of oversaturation issues of the Pico Flexx
    int range=50;           //look in a tolerance range of 50cm
    for (val = offset; val < 256; val++)
    {
      if(histo[tileIdx][val] >5) {
        sum += histo[tileIdx][val];
      }
      if (val>range+offset) {
        if(histo[tileIdx][val-range] >5) {
          sum-=histo[tileIdx][val-range];
        }
      }
      if (sum >= minObjSizeThresh)
      break;
    }
    //WRITE the value in the Tile Matrix:
    ninePixMatrix[(tileIdx-8)*-1] = (val-255)*-1;       //Here two modification have to be done to have the right visual orientation in the end
  }

  /*ä hier muss ich wohl nochmla drehen weil das tile bil dsonst falsch ist
    for (int i = 0; i < 9; i++)
    {
      ninePixMatrix[i] =((tempMat[i]-255)*-1);            //umdrehen
    }
*/

  if(true){                                             //need this if-true for the mutex
    std::lock_guard<std::mutex> lock(depMutex);         //lock again for writing process
    tileImg= cv::Mat(3, 3, CV_8UC1, &ninePixMatrix);    //Write the matr
  }
}

// counting per 100 passed frames
if (frameCounter==100)
{
  loop++;
  frameCounter=0;
}
frameCounter++;


//WRITE VALUES TO GLOVE
if(true){                                             //need this if-true for the mutex
  std::lock_guard<std::mutex> lock(tileMutex);        //lock to write values to glove
  if (stopWritingVals!=true)                          //Check if values should be written
  writeValues(9, &(ninePixMatrix[0]));}               //WRITE
  getCycleDur();                                      //Calculate how long this cycle took
}

//Toggle to undistort the picture (lense produces fisheye view)
void DepthDataListener::toggleUndistort()
{
  std::lock_guard<std::mutex> lock(depMutex);
  undistortImage = !undistortImage;
}



//_______________PRIVATE_______________
float DepthDataListener::adjustDepthValue(float zValue, float max)
{
  //if max is 0 (e.g. poti is at 0): set all tiles to out of range / s55
  if (max==0)
  {
    return 255;
  }
  //if max is smaller: set all tiles to max
  if (zValue > max)
  {
    zValue = max;
  }
  //make ZValue relative to max and return 0-255
  float newZValue = zValue / max * 255.0f;
  return newZValue;
}

//ä
float DepthDataListener::adjustDepthValueForImage(float zValue, float max)
{
  if (zValue > max)
  {
    zValue = max;
  }
  float newZValue = zValue / max * 180.0f;
  newZValue = (newZValue - 180) * -1;
  newZValue = myHueChange(newZValue, -50);
  return newZValue;
}

//ä
void printOutput() {
  int millisSince=((int) millis()-millisFirstFrame)/1000;
  if (millisSince>0) {
    fps = ((loop*100)+frameCounter)/millisSince;
  }
  printf("frame:\t %i.%i \t fps: %i \n", loop,frameCounter,fps);
  printf("cycle:\t %ims \t pause: %ims \n", globalCycleTime,globalPauseTime);
  printf("Range:\t %.1f m\n\n\n", distThresh);
  //ASCII BILD AUSGABE
  if(true){
    std::lock_guard<std::mutex> lock(tileMutex);

    for (int y = 0; y < 3; y++)
    {
      printf("\t");

      for (int x = 0; x < 3; x++)
      {
        //int asciiVal=(ninePixMatrix[((y * 3 + x) - 8) * -1] - 255) * -1;
        int asciiVal=ninePixMatrix[y * 3 +x];

        if (asciiVal>250)
        {
          printf("\t-\t");
        }
        else
        {
          printf("\t%i\t",asciiVal);
        }
      }
      printf("\n\n\n");
    }
    printf("\n\n\n");
  }
}

//Function to pass Tiles Img to main.cpp
cv::Mat passNineFrame() {
  return tileImg;
}
//Function to pass Depth Img to main.cpp
cv::Mat passDepFrame() {
  std::lock_guard<std::mutex> lock(depMutex);
  depImgMod.create (cv::Size (width, height), CV_8UC3);
  depImg.copyTo(depImgMod);
  return depImgMod;
}
