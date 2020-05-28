#include "udp.hpp"

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <vector>

#include "camera.hpp"
#include "glove.hpp"
#include "poti.hpp"
#include "timelog.hpp"

using boost::asio::ip::udp;

//****************************************************************
//                        UDP BROADCASTING
//****************************************************************
// UDP Broadcasting Class:
// runs independently from main udp_server class to regularly send out
// a "I am online" notification to all clients
udp_brodcasting::udp_brodcasting(boost::asio::io_service &io_service)
    : broad_socket_(io_service, udp::endpoint(udp::v4(), 9007)) {
  broad_socket_.open(udp::v4(), error);
  broad_socket_.set_option(udp::socket::reuse_address(true));
  broad_socket_.set_option(boost::asio::socket_base::broadcast(true));
  broad_endpoint_ =
      udp::endpoint(boost::asio::ip::address_v4::broadcast(), 9008);
}
void udp_brodcasting::sendBroadcast() {
  if (!error) {
    broad_socket_.send_to(boost::asio::buffer("Unfolding 1"), broad_endpoint_,
                          0, error);
  } else {
    broad_socket_.send_to(boost::asio::buffer("Unfolding 1"), broad_endpoint_,
                          0, error);
  }
}

//****************************************************************
//                          UDP CLIENT
//****************************************************************
udp_client::udp_client(udp::endpoint e) {
  endpoint = e;
  counter = 0;
  maxCount = 50;
  isActive = false;
}

// increment the counter and check if it is below max frames before drop
void udp_client::incCounter() {
  if (counter > maxCount) {
    isActive = false;
  } else {
    counter++;
  }
}

void udp_client::resetCounter() {
  counter = 0;
  isActive = true;
}
// check if this one is active
bool udp_client::checkState() { return isActive; }

bool udp_client::isEqual(udp::endpoint *checkEndpoint) {
  return endpoint == *checkEndpoint;
}

//****************************************************************
//                         UDP SERVER
//****************************************************************

// UDP Server Class
// This class receives requests from clients and also handlese the
// replies to them. Receiving and Sendung should happen in two threads.

udp_server::udp_server(boost::asio::io_service &io_service, int max)
    : strand_(io_service),
      socket_(io_service, udp::endpoint(udp::v4(), 9009)),
      udpRecLog(20),
      udpSendLog(20) {
  strand_.post(strand_.wrap(boost::bind(&udp_server::start_receive, this)));
  imgSize = 0;
  sendImg = false;
  maxClients = max;
}
// This is called externally whenever there is a new frame
void udp_server::postSend() {
  strand_.post(strand_.wrap(boost::bind(&udp_server::packAndSend, this)));
}

void udp_server::packAndSend() {
  udpSendLog.store("-");
  for (size_t i = 0; i < udpClient.size(); i++) {
    // send the ready packed string with all the values from packValStr
    // create new timer for next send call
    udpSendLog.store("-");
    if (udpClient[i].checkState()) {
      udpClient[i].incCounter();
      boost::shared_ptr<std::string> message(
          new std::string(packValStr(sendImg, imgSize)));
      socket_.async_send_to(
          boost::asio::buffer(*message), udpClient[i].endpoint,
          boost::bind(&udp_server::handle_send, this, message,
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));
      udpSendLog.store("after send");
    } else {
      // Delete nonactive instance by counting from the first one
      udpClient.erase(udpClient.begin() + i);
    }
  }
  udpSendLog.print("Pack and Send", "us", "ms");
}

void udp_server::start_receive() {
  // Look out for calls on port 9009
  socket_.async_receive_from(
      boost::asio::buffer(recv_buffer_), remote_endpoint_,
      boost::bind(&udp_server::handle_receive, this,
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}
// if there is a ne request...
void udp_server::handle_receive(const boost::system::error_code &error,
                                std::size_t /*bytes_transferred*/) {
  udpRecLog.store("-");
  int i = 0;
  bool inList = false;
  for (size_t i = 0; i < udpClient.size(); ++i) {
    if (udpClient[i].isEqual(&remote_endpoint_)) {
      udpClient[i].resetCounter();
      inList = true;
      break;
    }
  }

  if (!inList && udpClient.size() < maxClients) {
    // put it in the first not-active slot
    udpClient.push_back(udp_client(remote_endpoint_));
    // reset counter and send active...
    udpClient[udpClient.size() - 1].resetCounter();
  }

  udpRecLog.store("manage clients");

  sendImg = false;
  imgSize = 0;

  int incSize = std::find(recv_buffer_.begin(), recv_buffer_.end(), '\0') -
                recv_buffer_.begin();

  if (incSize > 0) {
    // printf("not empty\n");
    auto incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'i');
    if (incoming != recv_buffer_.end()) {
      int tmp = (*std::next(incoming, 1) - 48);
      sendImg = true;
      imgSize = tmp;
    }
    incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'm');
    if (incoming != recv_buffer_.end()) {
      motorsMuted = !motorsMuted;
      testMotors = false;
      muteAll();
    }
    incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 't');
    if (incoming != recv_buffer_.end()) {
      testMotors = !testMotors;
      motorsMuted = testMotors;
      muteAll();
    }

    incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'z');
    if (incoming != recv_buffer_.end()) {
      int tmp = (*std::next(incoming, 1) - 48);
      motorTestMatrix[tmp] = motorTestMatrix[tmp] == 0 ? 254 : 0;
      // motorTestMatrix[tmp] = abs(motorTestMatrix[tmp] - 255);
    }

    incoming = std::find(recv_buffer_.begin(), recv_buffer_.end(), 'c');
    if (incoming != recv_buffer_.end()) {
      calibRunning = true;
      muteAll();
      motorsMuted = true;
      doCalibration();
      calibRunning = false;
    }
  }
  udpRecLog.store("deciding on action");

  if (!error || error == boost::asio::error::message_size) {
    // bool flag?
  }

  // start listening again
  strand_.post(strand_.wrap(boost::bind(&udp_server::start_receive, this)));
  udpRecLog.store("post new receive");
  udpRecLog.print("Receiving one Frame", "us", "ms");
}

// Invoked after sending message
void udp_server::handle_send(boost::shared_ptr<std::string> /*message*/,
                             const boost::system::error_code & /*error*/,
                             std::size_t /*bytes_transferred*/) {}

//________ HELPERs_______
template <typename T>
std::string udp_server::add(unsigned char id, T const &value) {
  std::ostringstream oss;
  oss << id << ":" << std::to_string(value) << "|";
  std::string s = oss.str();
  // printf(id);
  return s;
}

// Send the Set of Data to the Android App
std::string udp_server::packValStr(bool sendImg, int imgSize) {
  std::string tmpStr;
  // Add the values to the string. TODO: slim everything a bit down? Does string
  // cost time?
  udpSendLog.store("pack in");
  tmpStr += add(0, frameCounter);          // c : c + | listener -> onnewData
  tmpStr += add(1, timeSinceLastNewData);  // c : m + | listener -> onNewData
  tmpStr += add(2, longestTimeNoData);     // c : m + | listener –> onNewData
  tmpStr += add(3, (int)fps);              // c : c - |
  tmpStr += add(4, globalPotiVal);         // p : c - |
  tmpStr += add(5, coreTempDouble);        // c : m ? | udp?
  tmpStr += add(6, connected);             // c : m + | newEventClass
  tmpStr += add(7, capturing);             // c : m + | newEventClass
  tmpStr += add(8, libraryCrashNo);        // c : m + | newEventClass
  tmpStr += add(9, droppedAtBridge);       // c : m + | newEventClass
  tmpStr += add(10, droppedAtFC);          // c : m + | newEventClass
  tmpStr += add(11, tenSecsDrops);         // c : m + | newEventClass
  tmpStr += add(12, deliveredFrames);      // c : m + | newEventClass
  tmpStr += add(13, 0);                    //:cycle sachen
  tmpStr += add(14, 0);                    //:cycle sachen
  tmpStr += add(15, motorsMuted);          // c : c + | glove?
  tmpStr += add(16, testMotors);           // c : m + | ?
  udpSendLog.store("pack vals");
  if (!testMotors) {
    std::lock_guard<std::mutex> lock(tilesMutex);
    for (size_t i = 0; i < 9; i++) {
      tmpStr += add(i + 17, tilesArray[i]);
    }
  } else {
    std::lock_guard<std::mutex> lock(motorTestMutex);
    for (size_t i = 0; i < 9; i++) {
      tmpStr += add(i + 17, motorTestMatrix[i]);
    }
  }
  udpSendLog.store("pack tiles");
  tmpStr += add(26, millis());
  // Send detailed picture if wanted
  if (sendImg) {
    tmpStr += "#|";
    cv::Mat dep;
    dep = passUdpFrame(imgSize);
    // cv::cvtColor(dep, dep, cv::COLOR_HSV2RGB, 3);
    cv::flip(dep, dep, -1);
    for (int i = 0; i < dep.rows; i++) {
      for (int j = 0; j < dep.cols; j++) {
        unsigned char t = dep.at<uchar>(i, j);
        tmpStr += t;
      }
    }
  }
  udpSendLog.store("pack image");
  return tmpStr;
}
