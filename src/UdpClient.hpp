#pragma once
#include <array>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <mutex>
#include <vector>

#include "Camera.hpp"
using namespace std::chrono;

//****************************************************************
//                          UDP CLIENT
//****************************************************************
class UdpClient {
public:
  UdpClient(boost::asio::ip::udp::endpoint e);
  void checkTimer();
  void resetTimer();
  bool checkState();
  bool isEqual(boost::asio::ip::udp::endpoint *checkEndpoint);
  boost::asio::ip::udp::endpoint endpoint;
  bool sendImg;
  int imgSize;

private:
  bool isActive;
  int maxTime;
  steady_clock::time_point lastCalled;
};