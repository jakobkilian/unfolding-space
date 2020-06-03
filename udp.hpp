#pragma once
#include <array>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <vector>
#include <mutex>

#include "camera.hpp"
#include "timelog.hpp"
using namespace std::chrono;

//****************************************************************
//                          UDP CLIENT
//****************************************************************
class udp_client {
 public:
  udp_client(boost::asio::ip::udp::endpoint e);
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

//****************************************************************
//                         UDP SERVER
//****************************************************************
class udp_server {
 public:
  udp_server(boost::asio::io_service &io_service, int max);
  // if it is a vector take this function
  void preparePacket(const std::string key,
                     const std::vector<unsigned char> data);
  void prepareImage();

  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint remote_endpoint_;
  boost::array<char, 4> recv_buffer_;

 private:
    std::mutex mux;
  static const int numClients = 5;
  bool _imgSend = false;
  std::vector<udp_client> udpClient;
  boost::asio::io_service::strand strand_;
  timelog udpRecLog;
  int maxClients;
  void sendPacket(int i, std::vector<unsigned char> vect);
  void start_receive();
  void handle_receive();
  void handle_send(boost::shared_ptr<std::string>);
  // broadcasting
  void broadcast();
  // timer checking
  void checkClientTimers();
  boost::asio::ip::udp::socket broad_socket_;
  boost::asio::ip::udp::endpoint broad_endpoint_;
  boost::system::error_code errorBroad;
  boost::system::error_code errorRec;
  boost::asio::deadline_timer timer1_;
  boost::asio::deadline_timer timer2_;
  DepthDataUtilities ddUtilities;

 public:
  // Template Function that recieves a any type of data (T const &data) and puts
  // it into a vector that can be passed by value
  template <typename T>
  constexpr void preparePacket(const std::string key, T const &data) {
    // only use some types: int, float, char array

    static_assert(
        std::is_same<T, int>::value || std::is_same<T, unsigned int>::value ||
            std::is_same<T, float>::value || std::is_same<T, char>::value ||
            std::is_same<T, unsigned char>::value ||
            std::is_same<T, float>::value || std::is_same<T, bool>::value,
        "MyError: This function can only be used with the following data "
        "types: int, "
        "unsigned int, float, char, unsigned char and "
        "std::vector<unsigned char> ");

    const int keyLength = key.length();
    const int dataSize = sizeof(data);
    const int packetLength = keyLength + 1 + dataSize;  //+1 for the ':'
    unsigned char *ptr = (unsigned char *)&data;
    std::vector<unsigned char> dataVect;
    for (int i = 0; i < keyLength; i++) {
      dataVect.push_back(key[i]);
    }
    // add ':' delimiter that marks end of key
    dataVect.push_back(':');
    for (int i = 0; i < dataSize; i++) {
      dataVect.push_back(*(ptr + i));
    }

    // Iterate through all online clients and post the sendPacket function
    for (size_t i = 0; i < udpClient.size(); i++) {
      if (udpClient[i].checkState()) {
        strand_.post(strand_.wrap(
            std::bind(&udp_server::sendPacket, this, i, dataVect)));
      }
      // throw out inactive ones
      else {
        // Delete nonactive instance by counting from the first one
        udpClient.erase(udpClient.begin() + i);
      }
    }
  }
};