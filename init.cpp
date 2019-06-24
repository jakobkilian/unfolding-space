#include "poti.hpp"
#include "camera.hpp"
#include "glove.hpp"
#include "init.hpp"


//from main: create the windows for the visual output und position them on the screen
void createWindows(){
  cv::namedWindow ("tileImg8", cv::WINDOW_NORMAL);
  cv::resizeWindow("tileImg8", 672,513);
  cv::moveWindow("tileImg8", 700, 50);
  cv::namedWindow ("depImg8", cv::WINDOW_NORMAL);
  cv::resizeWindow("depImg8", 672,513);
  cv::moveWindow("depImg8", 5, 50);
}


bool checkCam(){
    royale::CameraManager manager;
    royale::Vector<royale::String> camlist;
    camlist= manager.getConnectedCameraList();
    if (!camlist.empty())
    {
  return true;
    }
    return false;
}
