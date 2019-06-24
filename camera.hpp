#pragma once

#include <royale.hpp>
#include <opencv2/opencv.hpp>
#include <mutex>
#include <string.h>




void printCurTime(const std::string&);
cv::Mat passDepFrame();
cv::Mat passNineFrame();
void printOutput();


extern int globalCycleTime;
extern int globalPauseTime;
extern bool stopWritingVals;
extern bool noChangeInMatrix;
extern bool newDepthImage;
extern bool processingImg;
extern std::mutex depMutex;
extern long lastNewData;
extern int frameCounter;
extern int loop;
extern int fps;
extern std::array<uint8_t,9> ninePixMatrix;

extern bool gui;


class DepthDataListener : public royale::IDepthDataListener
{
        // lens matrices used for the undistortion of
        // the image
        cv::Mat cameraMatrix;
        cv::Mat distortionCoefficients;
        bool undistortImage = false;

    public:
        DepthDataListener() : undistortImage(false) {}
        void onNewData(const royale::DepthData *data);
        void setLensParameters(const royale::LensParameters &lensParameters);
        void toggleUndistort();

    private:
        float adjustDepthValue(float zValue, float max);
        float adjustDepthValueForImage(float zValue, float max);
};
