#include "UdpServer.hpp"

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <ctime>
#include <vector>

#include "Camera.hpp"
#include "Globals.hpp"
#include "MotorBoard.hpp"
#include "TimeLogger.hpp"
#include "UdpClient.hpp"

using boost::asio::ip::udp;
using namespace std::chrono;

//
/******************************************************************************
 *                                 UDP SERVER
 *                               ***************
 * UDP Server Class
 * This class broadcasts its online status, receives requests from clients and
 * also handlese the replies to them. Concurrency is managed by a
 * boost asio strand which subsequently invokes the tasks
 ******************************************************************************/
UdpServer::UdpServer(boost::asio::io_service &io_service, int max)
    : socket_(io_service, udp::endpoint(udp::v4(), 9009)), strand_(io_service),
      maxClients(max),
      broad_socket_(io_service, udp::endpoint(udp::v4(), 9007)),
      timer1_(io_service, boost::posix_time::milliseconds(500)),
      timer2_(io_service, boost::posix_time::milliseconds(500)) {
  // std::lock_guard<std::mutex> l(mux);
  // invoke first broadcast
  timer1_.async_wait(strand_.wrap(std::bind(&UdpServer::broadcast, this)));
  // invoke first client timer check
  timer2_.async_wait(
      strand_.wrap(std::bind(&UdpServer::checkClientTimers, this)));
  // invoke first receive
  strand_.post(strand_.wrap(std::bind(&UdpServer::start_receive, this)));
  // Open second Socket for broadcasting
  broad_socket_.open(udp::v4(), errorBroad);
  broad_socket_.set_option(udp::socket::reuse_address(true));
  broad_socket_.set_option(boost::asio::socket_base::broadcast(true));
  broad_endpoint_ =
      udp::endpoint(boost::asio::ip::address_v4::broadcast(), 9008);
}

//_______ Broadcast Online Status _______
void UdpServer::broadcast() {
  // std::lock_guard<std::mutex> l(mux);
  broad_socket_.send_to(boost::asio::buffer("Unfolding Glove " + std::to_string(Glob::modes.a_identifier)), broad_endpoint_, 0,
                        errorBroad);
  timer1_.expires_at(timer1_.expires_at() + boost::posix_time::seconds(1));
  timer1_.async_wait(strand_.wrap(std::bind(&UdpServer::broadcast, this)));
}

//_______ Broadcast Online Status _______
void UdpServer::checkClientTimers() {
  // std::lock_guard<std::mutex> l(mux);
  for (size_t i = 0; i < udpClient.size(); i++) {
    udpClient[i].checkTimer();
  }
  timer2_.expires_at(timer2_.expires_at() + boost::posix_time::seconds(1));
  timer2_.async_wait(
      strand_.wrap(std::bind(&UdpServer::checkClientTimers, this)));
}

void UdpServer::prepareImage() {
  // std::lock_guard<std::mutex> l(mux);
  // iterte through all active clients
  for (size_t i = 0; i < udpClient.size(); i++) {
    if (udpClient[i].checkState()) {
      if (udpClient[i].sendImg) {
        cv::Mat dep;
        dep = ddUtilities.getResizedDepthImage(udpClient[i].imgSize);
        // cv::cvtColor(dep, dep, cv::COLOR_HSV2RGB, 3);
        cv::flip(dep, dep, -1);
        std::vector<unsigned char> vect;
        vect.push_back('i');
        vect.push_back('m');
        vect.push_back('g');
        vect.push_back(':');
        for (int h = 0; h < dep.rows; h++) {
          for (int j = 0; j < dep.cols; j++) {
            vect.push_back(*(unsigned char *)(dep.data + h * dep.step + j));
          }
        }
        strand_.post(
            strand_.wrap(std::bind(&UdpServer::sendPacket, this, i, vect)));
      }
    }
    // throw out inactive ones
    else {
      // Delete nonactive instance by counting from the first one
      udpClient.erase(udpClient.begin() + i);
    }
  }
}

// Note: void preparePacket is defined in UdpServer.hpp as it is a Template
// Function
void UdpServer::preparePacket(const std::string key,
                              const std::vector<unsigned char> data) {
  // std::lock_guard<std::mutex> l(mux);
  const int keyLength = key.length();
  const int dataSize = data.size();
  std::vector<unsigned char> dataVect;
  for (int i = 0; i < keyLength; i++) {
    dataVect.push_back(key[i]);
  }
  // add ':' delimiter that marks end of key
  dataVect.push_back(':');
  for (int i = 0; i < dataSize; i++) {
    dataVect.push_back(data[i]);
  }

  // Iterate through all online clients and post the sendPacket function
  for (size_t i = 0; i < udpClient.size(); i++) {
    if (udpClient[i].checkState()) {
      strand_.post(
          strand_.wrap(std::bind(&UdpServer::sendPacket, this, i, dataVect)));
    }
    // throw out inactive ones
    else {
      // Delete nonactive instance by counting from the first one
      udpClient.erase(udpClient.begin() + i);
    }
  }
}

// send the whole vector to the clients via udp
void UdpServer::sendPacket(int id, std::vector<unsigned char> vect) {
  // std::lock_guard<std::mutex> l(mux);
  boost::shared_ptr<std::string> message(new std::string(""));
  socket_.async_send_to(boost::asio::buffer(vect), udpClient[id].endpoint,
                        std::bind(&UdpServer::handle_send, this, message));
}

//_______ Set Socket to Receiving _______
void UdpServer::start_receive() {
  // std::lock_guard<std::mutex> l(mux);
  // Look out for calls on port 9009
  socket_.async_receive_from(boost::asio::buffer(recv_buffer_),
                             remote_endpoint_,
                             std::bind(&UdpServer::handle_receive, this));
}

//_______ Handle Received Packets _______
void UdpServer::handle_receive() {
  // std::lock_guard<std::mutex> l(mux);
  udpRecLog.store("-");
  bool inList = false;
  int curClient = 0;
  udpRecLog.store("old client number: " + std::to_string(udpClient.size()));
  for (size_t i = 0; i < udpClient.size(); ++i) {
    if (udpClient[i].isEqual(&remote_endpoint_)) {
      udpClient[i].resetTimer();
      inList = true;
      curClient = i;
      break;
    }
  }

  if (!inList && udpClient.size() < maxClients) {
    // put it in the first not-active slot
    udpClient.push_back(UdpClient(remote_endpoint_));
    // reset counter and send active...
    udpClient[udpClient.size() - 1].resetTimer();
    curClient = udpClient.size() - 1;
  }

  udpRecLog.store("new client number: " + std::to_string(udpClient.size()));
  // reset local imagSend variable (if msg contains "i" it wil be set to true)
  _imgSend = false;
  int incSize = std::find(recv_buffer_.begin(), recv_buffer_.end(), '\0') -
                recv_buffer_.begin();

  if (incSize > 0) {
    // printf("not empty\n");
    auto incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'i');
    if (incoming != recv_buffer_.end()) {
      int tmp = (*std::next(incoming, 1) - 48);
      // set local variable to true
      _imgSend = true;

      if (tmp != udpClient[curClient].imgSize) {
        udpClient[curClient].imgSize = tmp;
      }
    }
    incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'm');
    if (incoming != recv_buffer_.end()) {
      Glob::modes.a_muted = !Glob::modes.a_muted;
      Glob::modes.a_testMode = false;
      Glob::motorBoard.muteAll();
    }
    incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 't');
    if (incoming != recv_buffer_.end()) {
      bool tempTest = Glob::modes.a_testMode;
      Glob::modes.a_testMode = !tempTest;
      Glob::modes.a_muted = tempTest;
      Glob::motorBoard.muteAll();
    }

    incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'z');
    if (incoming != recv_buffer_.end()) {
      int tmp = (*std::next(incoming, 1) - 48);
      {
        std::lock_guard<std::mutex> lock(Glob::motors.mut);
        Glob::motors.testTiles[tmp] =
            Glob::motors.testTiles[tmp] == 0 ? 254 : 0;
      }
    }

    incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'u');
    if (incoming != recv_buffer_.end()) {
      int tmp = (*std::next(incoming, 1) - 48);
      Glob::modes.a_cameraUseCase = tmp;
      Glob::a_restartUnfoldingFlag = true; // jump back to the beginning
    }

    incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'c');
    if (incoming != recv_buffer_.end()) {
      Glob::royalStats.a_isCalibRunning = true;
      Glob::motorBoard.muteAll();
      Glob::modes.a_muted = true;
      Glob::motorBoard.runCalib();
      Glob::royalStats.a_isCalibRunning = false;
    }
  }
  // Set imgSend if there's a diff to the last frame
  if (_imgSend != udpClient[curClient].sendImg) {
    udpClient[curClient].sendImg = _imgSend;
  }

  udpRecLog.store("deciding on action");

  if (!errorRec || errorRec == boost::asio::error::message_size) {
    // bool flag?
  }

  // start listening again
  strand_.post(strand_.wrap(std::bind(&UdpServer::start_receive, this)));
  udpRecLog.store("post new receive");
  udpRecLog.printAll("Receiving one Frame", "us", "ms");
  udpRecLog.reset();
}

// Invoked after sending message (dont delete)
void UdpServer::handle_send(
    __attribute__((unused)) boost::shared_ptr<std::string> message) {}
//                                    _____
//                                 [udp server]
//____________________________________________________________________________