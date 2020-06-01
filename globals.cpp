#include "globals.hpp"
#include "timelog.hpp"

#include <boost/asio.hpp>
#include <mutex>

boost::asio::io_service glob::udpSendService;
udp_server glob::udpSendServer(udpSendService, 3);
// All global mutex in this namespace start with "m_"
std::mutex glob::m_universal;  // Universal mutex to lock all items at once
bool glob::testBool = false;   // Muted Mode active?
bool glob::isMuted = false;
bool glob::isTestMode = false;  // Test Mode active?
std::mutex glob::m_testTiles;
unsigned char glob::testTiles[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
std::mutex glob::m_tiles;
unsigned char glob::tiles[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
int glob::imgSize = 5;
bool glob::sendImg = true;

timelog glob::newDataLog;

// Data and their Mutexes
cv::Mat glob::depImg;  // full depth image (one byte p. pixel)
std::mutex glob::depImgMutex;

royale::DepthData glob::dataCopy;  // storage of last depthFrame from libroyal
std::mutex glob::dataCopyMutex;

// Notifying processing data (pd) thread to end waiting
std::condition_variable glob::pdCond;  // depthData condition varibale
std::mutex glob::pdCondMutex;          // depthData condition varibale
bool glob::pdFlag = false;
int glob::lockFailCounter = 0;

// Notifying send values to motors (sv) thread to end waiting
std::condition_variable glob::svCond;  // send Values condition varibale
std::mutex glob::svCondMutex;          // send Values condition varibale
bool glob::svFlag = false;


RoyalStatus glob::royalStats;
PotiStatus glob::potiStats;