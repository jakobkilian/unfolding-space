#include "poti.hpp"
#include "camera.hpp"
#include "glove.hpp"


//from main: create the windows for the visual output und position them on the screen
void createWindows(){
  cv::namedWindow ("tileImg8", cv::WINDOW_NORMAL);
  cv::resizeWindow("tileImg8", 672,513);
  cv::moveWindow("tileImg8", 700, 50);
  cv::namedWindow ("depImg8", cv::WINDOW_NORMAL);
  cv::resizeWindow("depImg8", 672,513);
  cv::moveWindow("depImg8", 5, 50);
}
