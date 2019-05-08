#include "camera.hpp"
#include "glove.hpp"
#include "poti.hpp"
#include <sample_utils/EventReporter.hpp>
#include <array>

using std::cerr;
using std::cout;
using std::endl;


cv::VideoWriter video;

bool capturing=true;

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

int millisFirst;
bool first=false;

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
void printCycle()
{
  cycleTime+=millis()-lastPrintCurTime;
  globalCycleTime=cycleTime;
  //cout <<  " _________________________ Cycle Time:" << cycleTime <<  " ______________________________" << endl;
  cycleTime=0;
  lastPrintCurTime=millis();

}
//How long was the pause since last onNewData?
void printPause()
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


//This function gets called everytime there is a new depth frame!
void DepthDataListener::onNewData(const DepthData *data)
{


  lastNewData=millis();
  //printCurTime(" ");
  printPause();
  //printCurTime("Begin");
  //is this the first frame?

  distThresh=globalPotiVal/33.333333;     //get current distThresh

  width = data->width;
  height = data->height;
  int tileWidth = width / 3 + 1;
  int tileHeight = height / 3 + 1;

  if(first!=true) {
    millisFirst=millis();
    first=true;
  }

  //Check if saving and showing og img is finished
  /*    while (processingImg==true) {
  delay(1);
}
*/
int histo[9][256];

if(true){
  std::lock_guard<std::mutex> lock(depMutex);
  // create images which will be filled afterwards   |  CV_32FC3 heißt 32bit flaoting-point Array mit 3 Kanälen
  depImg.create (cv::Size (width, height), CV_8UC3);
  tileImg.create (cv::Size (3, 3), CV_8UC1);



  bzero(histo, sizeof(int) * 9 * 256);

  //printCurTime("Nur init Kram");
  for (int y = 0; y < height; y++)
  {

    uint8_t *depImgPtr = depImg.ptr<uint8_t>(y);
    for (int x = 0; x < width; x++) //iteriert durch die Spalten (also jeden einzelnen Pixel)
    {


      //data müssten die rohen daten der Kamera sein (werden oben der onNEwDate übergeben
      //d.h. hier wird ein bestimmter Wert (tempPixel) aus dem Datenarray (data) ausgegeben und in curPoint gespeichert
      auto curPoint = data->points.at(y * width + x);
      bool valid=curPoint.depthConfidence > 10;
      int tileIdx = (x / tileWidth) + 3 * (y / tileHeight);


      //confidence geht von 0 bis 255 und bezeichnet wie vertrauenwürdig ein punkt ist. 0 heißt ungültig
      if (valid)
      {
        // if the point is valid, map the pixel from 3D world
        // coordinates to a 2D plane (this will distort the image)
        uint8_t depth = (uint8_t)adjustDepthValue(curPoint.z, distThresh);      //Depth Value as 0-255 float in relation to distThresh
        histo[tileIdx][depth]++;

        depImgPtr[x * 3] = adjustDepthValueForImage(curPoint.z, distThresh);    //Hue value from 0-180
        depImgPtr[x * 3 + 1] = 0;                                             //Saturation is always the same
        depImgPtr[x * 3 + 2] = adjustDepthValue(curPoint.z, distThresh);                                             //Brightness is always the same
      }

      //If pixel is exactly 255 (means it is out of range)
      if (adjustDepthValue(curPoint.z, distThresh)==255)
      {
        //make pixel white (unvisible)
        depImgPtr[x * 3] = 255;
        depImgPtr[x * 3 + 1] = 0;
        depImgPtr[x * 3 + 2] = 255;
      }

      //If pixel is not valid (means confidence is too low)
      if (!valid)
      {
        //make pixel grey
        depImgPtr[x * 3] = 7;
        depImgPtr[x * 3 + 1] = 10;
        depImgPtr[x * 3 + 2] = 245;

        histo[tileIdx][255]++;

      }

    }
  }
}
newDepthImage=true;

//cv::resize(depImg, depImg, myWindowSize, cv::INTER_NEAREST);
//printCurTime("Resize");
//cv::cvtColor(depImg, depImg, cv::COLOR_HSV2RGB, 3);
//printCurTime("color");
//cv::flip(depImg,depImg, -1);
//printCurTime("Flip");
printCurTime("Init Kram und Data in Array und Img füllen");

if(true){
  std::lock_guard<std::mutex> lock(tileMutex);
  // find closest object per tile
  for (int tileIdx = 0; tileIdx < 9; tileIdx++)
  {
    int sum = 0;
    int val;
    int offset=17;          //exclude the first 17cm because of oversaturation issues
    int range=50;           //only look at a range of 0.5m
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
    ninePixMatrix[tileIdx] = val;
  }

  //printCurTime("Tiles Analyse");



  uint8_t tempMat[9];
  //Einmal umdrehen
  for (int i = 0; i < 9; i++)
  {
    tempMat[i] = ninePixMatrix[(i-8)*-1];
    //ninePixMatrix[i] = (ninePixMatrix[i] - 255) * -1;
  }



  //printCurTime("umdrehen I");

  for (int i = 0; i < 9; i++)
  {
    ninePixMatrix[i] =((tempMat[i]-255)*-1);            //umdrehen
  }




  if(true){
    std::lock_guard<std::mutex> lock(depMutex);
    tileImg= cv::Mat(3, 3, CV_8UC1, &tempMat);
  }
}

// counting per 100 passed frames
if (frameCounter==100)
{
  loop++;
  frameCounter=0;
}



//Printe das ASCII-Bild und andere Informationen
//passiert nun in der Main
//printOutput();



frameCounter++;
//Write the Values to the glove
//printCurTime("-");
if(true){
  std::lock_guard<std::mutex> lock(tileMutex);
  if (stopWritingVals!=true)
  writeValues(9, &(ninePixMatrix[0]));}
  printCurTime("Vorbereitung und Werte an den Handschuh");
  //delay(20);
  //printCurTime("Delay");
  printCycle();
}

void DepthDataListener::toggleUndistort()
{
  std::lock_guard<std::mutex> lock(depMutex);
  undistortImage = !undistortImage;
}

//_______________PRIVATE_______________

float DepthDataListener::adjustDepthValue(float zValue, float max)
{
  //if distThresh is zero, everything is out of range (255)
  if (max==0)
  {
    return 255;
  }

  if (zValue > max)
  {
    zValue = max;
  }
  float newZValue = zValue / max * 255.0f;
  return newZValue;
}

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

void printOutput() {
  int millisSince=((int) millis()-millisFirst)/1000;
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


cv::Mat passNineFrame() {


  return tileImg;
}
cv::Mat passDepFrame() {

  std::lock_guard<std::mutex> lock(depMutex);
  depImgMod.create (cv::Size (width, height), CV_8UC3);
  depImg.copyTo(depImgMod);
  return depImgMod;
}
