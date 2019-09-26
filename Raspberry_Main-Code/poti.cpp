/*
 * File: poti.cpp
 * Author: Jakob Kilian
 * Date: 26.09.19
 * Info: Funtions that initialize Poti and get value of it
 * Project: unfoldingspace.jakobkilian.de
 */

//----------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------
#include "poti.hpp"
#include "camera.hpp"
#include "glove.hpp"

//----------------------------------------------------------------------
// DECLARATIONS AND VARIABLES
//----------------------------------------------------------------------
int lastVal = 70;
int charsToRead = 0;
char thechar = ' ';
char lastThechar = ' ';
struct usb_bus *bus = NULL;
struct usb_device *digiSpark = NULL;
struct usb_device *device = NULL;

//----------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------

//________________________________________________
// Raspi has no analog in. I used a small Arduino (Digispark) to send poti vlaue
// to the raspi, which has to be initialzied. Gets called in the main.cpp
void initPoti() {
  // Initialize the USB library
  usb_init();
  // Enumerate the USB device tree
  usb_find_busses();
  usb_find_devices();
  // Iterate through attached busses and devices
  bus = usb_get_busses();
  while (bus != NULL) {
    device = bus->devices;
    while (device != NULL) {
      // Check to see if each USB device matches the DigiSpark Vendor and
      // Product IDs
      if ((device->descriptor.idVendor == 0x16c0) &&
          (device->descriptor.idProduct == 0x05df)) {
        digiSpark = device;
        std::cout << "FOUND Digispark" << '\n';
      } else {
        std::cout << "No digiSpark there" << '\n';
      }
      device = device->next;
    }
    bus = bus->next;
  }
}

//________________________________________________
int updatePoti() {
  // printf("do a poti check\n");
  thechar = ' '; // reset char
  // If a digiSpark was found
  if (digiSpark != NULL) {
    int result = 0;
    int i = 0;
    // int numInterfaces = 0;
    struct usb_dev_handle *devHandle = NULL;
    struct usb_interface_descriptor *interface = NULL;
    devHandle = usb_open(digiSpark);
    if (devHandle != NULL) {
      // printf("get some stuff\n");
      interface = &(digiSpark->config->interface[0].altsetting[0]);
      // Try to read from the digispark
      while (thechar != 4) {
        thechar = 4;
        result = usb_control_msg(devHandle, (0x01 << 5) | 0x80, 0x01, 0, 0,
                                 &thechar, 1, 1000);
        if (result < 0) {
          break;
        }
        // The following lines get the Poti Value (0-99) from the chars. There
        // might be an easier way...
        else {
          if (thechar > '/' && thechar < ':') {
            lastVal = thechar - '0';
            if (lastThechar > '/' && lastThechar < ':') {
              lastVal = (lastThechar - '0') * 10 + lastVal;
            }
          }
          lastThechar = thechar;
          i++;
          if (i >= charsToRead && charsToRead > 0) {
            // printf("poti wert: %c | %c | %i \n",
            // thechar,lastThechar,lastVal);
            break;
          }
        }
      }
      result = usb_release_interface(devHandle, interface->bInterfaceNumber);
      usb_close(devHandle);
    }
  } else {
    std::cout << "can't get new poti val" << '\n';
    return -1;
  }
  globalPotiVal = lastVal;
  return lastVal;
}
