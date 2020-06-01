#pragma once

//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include <string.h>

#include <condition_variable>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <royale.hpp>
#include <thread>
#include <royale/IEvent.hpp>


#include "timelog.hpp"

using namespace std::chrono;
extern long lastNewData;
extern int frameCounter;

//----------------------------------------------------------------------
// CLASSES
//----------------------------------------------------------------------
class DepthDataListener : public royale::IDepthDataListener {
 public:
  void onNewData(const royale::DepthData *data);
  void processData();
  void setLensParameters(const royale::LensParameters &lensParameters);

 private:
  uint8_t adjustDepthValue(float zValue, float max);
  float adjustDepthValueForImage(float zValue, float max);
};





//________________________________________________
// Gets called by Royale irregularily.
// Holds the camera state, errors and info about drops
class EventReporter : public royale::IEventListener {
 public:
  virtual ~EventReporter() = default;

virtual void onEvent(std::unique_ptr<royale::IEvent> &&event) override;
private:
void extractDrops(royale::String str);

  };



  class DepthDataUtilities {
    public:
    void processData();
    cv::Mat getResizedDepthImage(int);
  };