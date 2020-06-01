#include "globals.hpp"

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
int glob::imgSize=5;
bool glob::sendImg=true;
//does the system use a distance potentiometer?
bool potiAvailable=false;

RoyalStatus glob::royalStats; 




