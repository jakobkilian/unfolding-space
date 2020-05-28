#pragma once

#include <array>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <vector>
#include "timelog.hpp"


//****************************************************************
//                        UDP BROADCASTING
//****************************************************************
class udp_brodcasting {
 public:
  udp_brodcasting(boost::asio::io_service &io_service);
  boost::asio::ip::udp::socket broad_socket_;
  boost::asio::ip::udp::endpoint broad_endpoint_;
  boost::system::error_code error;

 public:
  void sendBroadcast();
};

//****************************************************************
//                          UDP CLIENT
//****************************************************************
class udp_client {
 public:
  udp_client(boost::asio::ip::udp::endpoint e);
  void incCounter();
  void resetCounter();
  bool checkState();
  bool isEqual(boost::asio::ip::udp::endpoint *checkEndpoint);
  boost::asio::ip::udp::endpoint endpoint;

 private:
  int counter;
  bool isActive;
  int maxCount;
};

//****************************************************************
//                         UDP SERVER
//****************************************************************
class udp_server {
 public:
  udp_server(boost::asio::io_service &io_service, int max);
  void postSend();
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint remote_endpoint_;
  boost::array<char, 4> recv_buffer_;

 private:
  static const int numClients = 5;
  std::vector<udp_client> udpClient;
  boost::asio::io_service::strand strand_;
  timelog udpRecLog;
  timelog udpSendLog;
  bool sendImg;
  int imgSize;
  int maxClients;
  void packAndSend();
  void start_receive();
  void handle_receive(const boost::system::error_code &error, std::size_t);
  void handle_send(boost::shared_ptr<std::string>,
                   const boost::system::error_code &, std::size_t);

  // Helper
  template <typename T>
  std::string add(unsigned char id, T const &value);

  // Send the Set of Data to the Android App
  std::string packValStr(bool sendImg, int imgSize);
};