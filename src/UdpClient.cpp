/* INFO
 * Every monitoring app requesting information acts as a client that gets added
 * to a list by the server as an UdpClient object. See Readme for details
 */

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

using boost::asio::ip::udp;
using namespace std::chrono;

/******************************************************************************
 *                                 UDP CLIENT
 *                               ***************
 * UDP Client Class
 * For each client that sends a data-request to the server an instance of this
 *class is generated to save the client's IP, Port and needs.
 ******************************************************************************/
UdpClient::UdpClient(udp::endpoint e) {
  endpoint = e;
  maxTime = 2000;
  isActive = false;
  lastCalled = steady_clock::now();
}
// increment the counter and check if it is below max frames before drop
void UdpClient::checkTimer() {
  if (std::chrono::duration_cast<std::chrono::milliseconds>(
          steady_clock::now() - lastCalled)
          .count() > maxTime) {
    isActive = false;
  }
}

void UdpClient::resetTimer() {
  lastCalled = steady_clock::now();
  isActive = true;
}

// check if this one is active
bool UdpClient::checkState() { return isActive; }

bool UdpClient::isEqual(udp::endpoint *checkEndpoint) {
  return endpoint == *checkEndpoint;
}
//                                    _____
//                                 [UDP CLIENT]
//____________________________________________________________________________