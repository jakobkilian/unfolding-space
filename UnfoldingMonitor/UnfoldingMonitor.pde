/*
 * File: UnfoldingMonitor.pde
 * Author: Jakob Kilian
 * Date: 26.09.19
 * Info: This is a Processing.org sketch for an android app that monitors the
 * glove's activity via udp
 * Project: unfoldingspace.jakobkilian.de
 */

// import UDP library
// library is attached (has to be installed in the processing libraries folder)
import hypermedia.net.*;

// SYSTEM_________
long[] lastCall = new long[4];   // saves the millis() of the last call
String[] inVal = new String[25]; // saves the millis() of the last call
int proFps;
int countFrames;

// SETTINGS_______
int callHz = 20; // how often should the Raspi be called?

// FUNCTIONS______
long mylong(String a) { return Long.parseLong(a); }
int myInt(String a) { return Integer.parseInt(a); }

// checks if there should be a new UDP call to the Raspberry
boolean myTimer(int thisHz, int thisId) {
  if ((millis() - lastCall[thisId]) > (1000 / thisHz)) {
    return true;
  } else {
    return false;
  }
}

void myText(String thisText, int thisRow, int thisColumn) {
  PVector txt0 = new PVector(50, 30);
  PVector txtGap = new PVector(50, 350);
  fill(255);
  textSize(45);
  if (thisText != null)
    text(thisText, txt0.y + (thisColumn * txtGap.y),
         txt0.x + (thisRow * txtGap.x));
}

// UDP STUFF_______
String current = "";
UDP udp;                     // define the UDP object
String left = "left";        // the message to send
String right = "right";      // the message to send
String ip = "192.168.1.255"; // the remote IP address
int inPort = 53333;          // the destination port
int outPort = 52222;         // the destination port
String message = "0";

// SETUP___________
void setup() {
  fullScreen();
  frameRate(50);
  noStroke();
  fill(0);
  // create a new datagram connection on port 6000
  // and wait for incomming message
  udp = new UDP(this, 53333);
  // udp.log( true );     // <-- printout the connection activity
  udp.listen(true);

  for (int i = 0; i < inVal.length; i++) {
    inVal[i] = "111";
  }
}

// DRAW___________
void draw() {
  // check timer
  if (myTimer(1, 0)) {
    lastCall[0] = millis();
    proFps = countFrames;
    countFrames = 0;
  }

  background(0);
  // get system time
  long sysTime = (System.currentTimeMillis() / 1000);
  // how long is the raspi offline?
  long offTime = sysTime - mylong(inVal[0]);

  // if longer than a second:
  if (offTime > 1) {
    fill(100, 20, 20);
    rect(0, 0, 600, 55);
    myText(str(offTime), 0, 1);
    myText("seconds", 0, 2);
  }
  // else: last connection now!
  else {
    fill(20, 100, 20);
    rect(0, 0, 600, 55);
    myText("online!", 0, 1);
  }
  myText("status:", 0, 0);

  myText("last frame:", 1, 0);
  myText(inVal[1] + " ms", 1, 1);
  myText("longest.:", 2, 0);
  myText(inVal[2] + " ms", 2, 1);
  myText("frames:", 3, 0);
  myText(inVal[3] + " fps", 3, 1);
  myText("distance:", 4, 0);
  if (inVal[4].length() > 3) {
    myText(inVal[4].substring(0, 3) + " m", 4, 1);
  }

  if (int(inVal[6]) == 1) {
    myText("connected", 6, 1);
  } else {
    fill(100, 20, 20);
    rect(0, 313, 600, 45);
    myText("--", 6, 1);
  }
  myText("camera", 6, 0);
  if (int(inVal[7]) == 1) {
    myText("running", 7, 1);
  } else {
    fill(100, 20, 20);
    rect(0, 363, 600, 45);
    myText("--", 7, 1);
  }
  myText("capture:", 7, 0);
  myText("lib crashed:", 8, 0);
  myText(inVal[8] + " mal", 8, 1);
  myText("drops bridge:", 9, 0);
  myText(inVal[9], 9, 1);
  myText("drops FC:", 10, 0);
  myText(inVal[10], 10, 1);
  myText("drops in 10s:", 11, 0);
  myText(inVal[11], 11, 1);
  myText("delivered:", 12, 0);
  myText(inVal[12] + "  frames", 12, 1);
  myText("cycle time:", 13, 0);
  myText(inVal[13] + " ms", 13, 1);
  myText("pause time:", 14, 0);
  myText(inVal[14] + " ms", 14, 1);

  myText("process fps:", 16, 0);
  myText(str(proFps), 16, 1);
  fill(255);
  rect(200, 1050, 1000, 1000);
  int c = 15;
  int tilSiz = 300;
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      textSize(30);
      fill(map(int(inVal[c]), 0, 255, 255, 0));
      rect(250 + (x * tilSiz), 1100 + (y * tilSiz), tilSiz, tilSiz);
      fill(255);
      text(inVal[c], 370 + (x * tilSiz), 1230 + (y * tilSiz));
      c++;
    }
  }
}


//gets called when there are incoming bytes
void receive(byte[] data, String ip, int inPort) { // <-- extended handler
  String temp = new String(data);
  int idDig = temp.indexOf(":");
  int id = myInt(temp.substring(0, idDig));
  inVal[id] = temp.substring(idDig + 1);
  if (id == 4) {
    inVal[4] = str(float(inVal[4]) / 33.333333333);
  }

  if (id == 0) {
    countFrames++;
  }
}
