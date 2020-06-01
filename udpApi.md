This a small cheat sheet concerning the exchange of dat between computing unit (CU) and control app via udp.
After a client *registers* at the server there are two types of messages: *Status Messages* coming from the CU and *Request / Control Messages* coming from the apps. Additionaly to UDP internal logics, no additional handshaking or matching is taking place.


# Registration
The control app catches an "online" message from a server in the local network by listening to udp port 9008. By catching the IP adress, the client can no start to send requests to this IP on port 9009.

# Request / Control Messages
Whenever a message comes in, a 2 sec(?) timer is set. During this period of time, the server sends out its Status Messages. If there is no new msg it stops after expiry.
These messages can contain characters that trigger events on the CU:

- i : requesting the full depth image 
    -> add a number right behind for the size. The scale goes from 1 to 9 and represents a respective resolution of 20 * 20, up to 180 * 180 pixels
- m : toggle muting of the motors. 
- t : toggle test mode (automatically mutes as well)
- z : toggle one motor (0 to 255 and back)
    -> add the motor number right behind (1-9)
- c : trigger calibration process of all motors


# Status Messages
Status Messages get triggered from various locations of the CU's code and at various points of time. Some (e.g. the core temperature value) get sent more occasionally, some get send every incoming frame.
Every message starts with a ascii encoded key for better readability delimited by a ':' (hex value: 3A). After that the data gets transferred in a plain uchar array. It is therefore crucial to know the data type of the message...
These are the keys:

- img           [byte][array]       pixel by pixel...
- motors        [byte][array]       motor by motor...
- frameCounter  [int]               sequential number incremented every frame
- coreTemp      [float]             Temperature of the Raspberry's core in ° C
- fps           [int]               Frames per second on the CU
- isConnected   [bool]              Is the Pico Flexx camera connected?
- isCapturing   [bool]              Is the Pico Flexx camera in capturing mode?
- libCrashes    [int]               How often did the royal library crash since startup?
- isTestMode    [bool]              CU is in Test Mode: the motors represent the test values not the camera values
- isMuted       [bool]              All motors are muted 
- drpBridge     [int]               Lib Royale: How many frames got dropped at the bridge during the last deptFrame calculation?
- drpFC         [int]               Lib Royale: How many frames got dropped at the FC during the last deptFrame calculation?
- delivFrames   [int]               Lib Royale: How many frames got finally delivered
- drpMinute     [int]               Lib Royale: Summation of all drops in the last minute