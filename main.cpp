#include <royale.hpp>
#include <opencv2/opencv.hpp>
#include <SerialStream.h>
#include <string>
#include <sample_utils/PlatformResources.hpp>

#define PORT "/dev/cu.usbmodem1411" //This is system-specific

using namespace royale;
using namespace sample_utils;
using namespace std;
using namespace cv;
using namespace LibSerial;

/* --Arduino Serial Stuff-- */

SerialStream ardu;
//char for concluding message
const char endMsg = 13;
//char for next value
const char nxtInt = ' ';
//lra actuator values send to the arduino (0-255)
string lraValues[9] = {"0", "0", "0", "0","0", "0", "0", "0","0"};
//mapping of the lra actuators
int lraMapping[9] = {2,5,8,1,4,7,0,3,6};


/* --image calculation-- */

//array for saving the 9 LRA-output values
uchar ninePixMatrix [3][3] = {};
//when scanning the depth image for closest object: the tolerance/range the scanner is checking (total range=255)
int depthScanTolerance=10;
//when scanning... : the min number of pixels, an object must have (smaller objects might be noise
int minObjSizeThresh=300;

float distThresh=2.8;
float myCrop=0.25;

/* --other-- */

//standard size of the Windows
Size_<int> myWindowSize= Size(650,500);



float myMap(float value, float istart, float istop, float ostart, float ostop) {
    return ostart + (ostop - ostart) * ((value - istart) / (istop - istart));
}

int myHueChange(float oldValue, float changeValue){
    int   oldInt= static_cast<int>(oldValue);
    int   changeInt= static_cast<int>(changeValue);
    return (oldInt+changeInt)%360;
}


//_______________ARDUINO FUNCTIONS________________

void openArdu()
{
    ardu.Open(PORT);
    /*The arduino must be setup to use the same baud rate*/
    ardu.SetBaudRate(SerialStreamBuf::BAUD_115200);
    ardu.SetCharSize(SerialStreamBuf::CHAR_SIZE_8);
    ardu.SetNumOfStopBits(1) ;
    ardu.SetParity( SerialStreamBuf::PARITY_ODD ) ;
    ardu.SetFlowControl( SerialStreamBuf::FLOW_CONTROL_HARD ) ;
    ardu.SetVTime(1);
    ardu.SetVMin(0);
    cout << "Arduino ready" << std::endl;
}

void closeArdu()
{
    ardu.Close() ;
}

void sendArdu()
{
    for (int i=0; i<8; i++) {
        ardu <<lraValues[lraMapping[i]] << nxtInt;                 //send value and nextInt
        // cout << lraValues[lraMapping[i]]  << " - "    ;          //print values, if needed
    }
    ardu << endMsg;                                 //print linebreak char to end the message (Arduino Messenger Library)
}



//_______________CAMERA LISTENER________________


class MyListener : public IDepthDataListener
{
    
    public :
    
    MyListener() :
    undistortImage (false)
    {
    }
    
    void onNewData (const DepthData *data)
    {
        // this callback function will be called for every new
        // depth frame
        
        std::lock_guard<std::mutex> lock (flagMutex);
        
        // create images which will be filled afterwards
        multiCh.create (Size (data->width, data->height), CV_32FC3);
        depImg.create (Size (data->width, data->height), CV_32FC3);
        
        
        // set the image to zero
        multiCh = Scalar::all (0);
        depImg = Scalar::all (0);
        
        
        
        int k = 0;
        for (int y = 0; y < multiCh.rows; y++)
        {
            float *multiChPtr = multiCh.ptr<float> (y);
            float *depImgPtr = depImg.ptr<float> (y);
            
            for (int x = 0; x < multiCh.cols; x++, k++)
            {
                //data müssten die rohen daten der Kamera sein (werden oben der onNEwDate übergeben
                //d.h. hier wird ein bestimmter Wert (k) aus dem Datenarray (data) ausgegeben und in curPoint gespeichert
                auto curPoint = data->points.at (k);
                
                //confidence geht von 0 bis 255 und bezeichnet wie vertrauenwürdig ein punkt ist. 0 heißt ungültig
                if (curPoint.depthConfidence > 0)
                {
                    // if the point is valid, map the pixel from 3D world
                    // coordinates to a 2D plane (this will distort the image)
                    multiChPtr[x*3] = adjustDepthValue (curPoint.z, distThresh);
                    multiChPtr[x*3+1] = curPoint.depthConfidence;
                    multiChPtr[x*3+2] = 200;
                    
                    depImgPtr[x*3] = adjustDepthValueForImage (curPoint.z, distThresh);
                    depImgPtr[x*3+1] = curPoint.depthConfidence;
                    depImgPtr[x*3+2] = 200;
                    
                    
                }
                
                else{
                    multiChPtr[x*3] = 255;
                    multiChPtr[x*3+1] = 0;
                    multiChPtr[x*3+2] = 255;
                    depImgPtr[x*3] = 255;
                    depImgPtr[x*3+1] = 0;
                    depImgPtr[x*3+2] = 255;
                }
                
                if (curPoint.z> distThresh-0.2){
                    multiChPtr[x*3] = 255;
                    multiChPtr[x*3+1] = 0;
                    multiChPtr[x*3+2] = 255;
                    depImgPtr[x*3] = 255;
                    depImgPtr[x*3+1] = 0;
                    depImgPtr[x*3+2] = 255;
                
                }
                
            }
        }
        
        
        // create images to store the 8Bit version (some OpenCV
        // functions may only work on 8Bit images)
        multiCh8.create (Size (data->width, data->height), CV_8UC3);
        depImg8.create (Size (data->width, data->height), CV_8UC3);
        fehler8.create (Size (data->width, data->height), CV_8UC3);
        
        if (undistortImage)
        {
            // call the undistortion function on the z image

            multiCh(Rect(myCrop*224/2,0,223-myCrop*224,170)).copyTo(multiCh);
            depImg(Rect(myCrop*224/2,0,223-myCrop*224,170)).copyTo(depImg);
            
        }
        
        
        
        
        
        
        
        
        multiCh.convertTo (multiCh8, CV_8UC3);
        
  
        
        depImg.convertTo (depImg8, CV_8UC3);
        
        flip(multiCh8, multiCh8, -1);
        flip(depImg8, depImg8, -1);
        
        
        
   
        
        /* --calculate the 9pixels matrix-- */
        int thisW=multiCh8.size().width;
        int thisH=multiCh8.size().height;
        
        for (int ox=0; ox<3; ox++) {
            for (int oy=0; oy<3; oy++) {
                
                int thisCounter=0;
                uchar thisPixel [4500]={};
                
                for (int i=0; i<4500; i++) {
                    thisPixel[i]=255;
                }
                
                for (int y =thisH/3*oy; y< thisH/3*(oy+1); y++) {
                    for (int x =thisW/3*ox; x< thisW/3*(ox+1); x++) {
                        
                        Vec3f werte = multiCh8.at<Vec3b>(y, x);
                        uchar thisDepth = werte.val[0];
                        uchar thisConf = werte.val[2];
                        
                        if (thisConf>0){
                            thisPixel[thisCounter]=thisDepth;
                            
                            //  cout << werte.val[0] << endl;
                            thisCounter++;
                            
                        }// if conf
                    } //for x
                }//for y
                
                
                //scanner
                
                //Depth Wert
                for (int d=0; d<255-depthScanTolerance; d++) {
                    
                    int anzImRange=0;
                    
                    //durch thisPixel iterieren
                    for (int pos=0; pos<4500; pos++) {
                        if (thisPixel[pos] >=d && thisPixel[pos] <d+depthScanTolerance){
                            anzImRange++;
                        }
                    }
                    //cout << anzImRange << " -  - " << d << endl;
                    
                    
                    if (anzImRange>minObjSizeThresh)
                    {
                        //smoothing
                        // int smoothingCalc=(ninePixMatrix[oy][ox]+d)/2;
                        ninePixMatrix[oy][ox]=d;
                        
                        
                        break;
                        
                    }
                    else{
                        //if the pixel has to low confidence - make it white
                        ninePixMatrix[oy][ox]=255;
                    }
                    
                    
                }//for dd
            }//for oy
        }//for ox
        
        
        
        Mat tempImg( 3,3, CV_8UC1, ninePixMatrix );
        //printf( "Decimal: \t%i\n", ninePixMatrix[0][0]);
        
        ninePixImg.create (Size (3,3), CV_8UC1);
        
        resize (tempImg, ninePixImg, Size(3,3) ,0,0, INTER_NEAREST);
        resize (ninePixImg, tempImg, myWindowSize ,0,0, INTER_NEAREST);
        imshow ("ninePixImg", tempImg);
        
        
        // convert images to the 8Bit version
        // This sample uses a fixed scaling of the values to (0, 255) to avoid flickering.
        // You can also replace this with an automatic scaling by using
        // normalize(grayImage, grayImage8, 0, 255, NORM_MINMAX, CV_8UC1
        
        

        
        
        
        
        cvtColor (depImg8, depImg8,COLOR_HSV2RGB, 3);
        resize (depImg8, depImg8,myWindowSize,INTER_LANCZOS4);
        
        
 
        
        
        imshow ("depImg8", depImg8);
        
        
        /* --fill the values in the Arduino LR value array-- */
        int myCounter=0;
        for (int y=0; y<3; y++) {
            for (int x=0; x<3; x++) {
                Scalar intensity = ninePixImg.at<uchar>(x, y);
                stringstream tempStr ;
                tempStr << intensity.val[0]/2; //divide to get a range from 0 to 128
                lraValues[myCounter] = tempStr.str();
                myCounter++;
            }
            
        }
        
        /* --send serial data to Arduino-- */
        sendArdu();
        
    }
    
    
    
    
    
    /* --settings for aamera-- */
    
    void setLensParameters (const LensParameters &lensParameters)
    {
        // Construct the camera matrix
        // (fx   0    cx)
        // (0    fy   cy)
        // (0    0    1 )
        cameraMatrix = (Mat1d (3, 3) << lensParameters.focalLength.first, 0, lensParameters.principalPoint.first,
                        0, lensParameters.focalLength.second, lensParameters.principalPoint.second,
                        0, 0, 1);
        
        // Construct the distortion coefficients
        // k1 k2 p1 p2 k3
        distortionCoefficients = (Mat1d (1, 5) << lensParameters.distortionRadial[0],
                                  lensParameters.distortionRadial[1],
                                  lensParameters.distortionTangential.first,
                                  lensParameters.distortionTangential.second,
                                  lensParameters.distortionRadial[2]);
    }
    
    
    
    
    
    
    void toggleUndistort()
    {
        std::lock_guard<std::mutex> lock (flagMutex);
        undistortImage = !undistortImage;
    }
    
    
    //_______________PRIVATE_______________
    
    
private:
    
    float adjustDepthValue (float zValue, float max)
    {
        if (zValue>max){zValue=max;}
        float newZValue = zValue / max * 255.0f;
        return newZValue;
        
    }
    
    
    
    float adjustDepthValueForImage (float zValue, float max)
    {
        if (zValue>max){zValue=max;}
        float newZValue = zValue / max * 180.0f;
        newZValue= myMap(newZValue, 0, 180, 180, 0);
        newZValue= myHueChange(newZValue, -50);
        return newZValue;
        
    }
    
    // define images for depth and gray
    // and for their 8Bit and scaled versions
    Mat multiCh, multiCh8, fehler, fehler8, ninePixImg, depImg, depImg8;
    
    // lens matrices used for the undistortion of
    // the image
    Mat cameraMatrix;
    Mat distortionCoefficients;
    
    std::mutex flagMutex;
        bool undistortImage=false;
};





//_______________MAIN LOOP________________


int main (int argc, char *argv[])
{
    // Windows requires that the application allocate these, not the DLL.
    PlatformResources resources;
    
    // This is the data listener which will receive callbacks.  It's declared
    // before the cameraDevice so that, if this function exits with a 'return'
    // statement while the camera is still capturing, it will still be in scope
    // until the cameraDevice's destructor implicitly de-registers the listener.
    MyListener listener;
    
    // this represents the main camera device object
    std::unique_ptr<ICameraDevice> cameraDevice;
    
    
    //open connection to Arduino
    openArdu();
    
    
    
    // the camera manager will query for a connected camera
    {
        CameraManager manager;
        
        // check the number of arguments
        if (argc > 1)
        {
            // if the program was called with an argument try to open this as a file
            cout << "Trying to open : " << argv[1] << endl;
            cameraDevice = manager.createCamera (argv[1]);
        }
        else
        {
            // if no argument was given try to open the first connected camera
            royale::Vector<royale::String> camlist (manager.getConnectedCameraList());
            cout << "Detected " << camlist.size() << " camera(s)." << endl;
            
            if (!camlist.empty())
            {
                cameraDevice = manager.createCamera (camlist[0]);
            }
            else
            {
                cerr << "No suitable camera device detected." << endl
                << "Please make sure that a supported camera is plugged in, all drivers are "
                << "installed, and you have proper USB permission" << endl;
                return 1;
            }
            
            camlist.clear();
        }
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
    if (status != CameraStatus::SUCCESS)
    {
        cerr << "Cannot initialize the camera device, error string : " << getErrorString (status) << endl;
        return 1;
    }
    
    // retrieve the lens parameters from Royale
    LensParameters lensParameters;
    status = cameraDevice->getLensParameters (lensParameters);
    if (status != CameraStatus::SUCCESS)
    {
        cerr << "Can't read out the lens parameters" << endl;
        return 1;
    }
    
    listener.setLensParameters (lensParameters);
    
    // register a data listener
    if (cameraDevice->registerDataListener (&listener) != CameraStatus::SUCCESS)
    {
        cerr << "Error registering data listener" << endl;
        return 1;
    }
    
    // create two windows
    namedWindow ("ninePixImg", WINDOW_AUTOSIZE);
    namedWindow ("depImg8", WINDOW_AUTOSIZE);
    
    moveWindow("depImg8", 50, 50);
    moveWindow("ninePixImg", 700, 50);

    
    //test
    
    
    
    // start capture mode
    
    //mein stuff: cameraDevice->setExposureMode(royale::ExposureMode AUTO);
    
    if (cameraDevice->startCapture() != CameraStatus::SUCCESS)
    {
        cerr << "Error starting the capturing" << endl;
        return 1;
    }
    
    
    int currentKey = 0;
    
    while (currentKey != 27)
    {
        // wait until a key is pressed
        currentKey = waitKey (0) & 255;
        
        if (currentKey == 'd')
        {
            // toggle the undistortion of the image
            listener.toggleUndistort();
        }
        
    }
    
    closeArdu();
    cout << "Arduino END" << std::endl;
    
    // stop capture mode
    if (cameraDevice->stopCapture() != CameraStatus::SUCCESS)
    {
        cerr << "Error stopping the capturing" << endl;
        return 1;
    }
    
    return 0;
}
