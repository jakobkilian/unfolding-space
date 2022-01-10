# The Unfolding Space Glove

***A Wearable Spatio-Visual to Haptic Sensory Substitution Device for Blind People***

[TOC]

## About the Project

The Unfolding Space Glove is an Open Source wearbale that **allows blind users to haptically sense the depth of their surrounding space** and thus (hopefully) better navigate through it. 

The device employs the concept of [Sensory Substitution](https://en.wikipedia.org/wiki/Sensory_substitution), which in simple terms states that if one sensory modality is missing, the brain is able to receive and process the missing information by means of another modality. There has been a great deal of research on this topic over the last 50 years, but as yet there is no widely used device on the market that implements these ideas. Initiated in 2018 as an interaction design project as part of an undergraduate thesis, several prorotypes have been developed over the years, aiming to learn from the mistakes of other projects and use design methods to develop a more user-friendly device; the last one was tested in 2021 in a study with blind and sighted (but blindfolded) subjects (see section Publication). 

### Publication

For February/March 2022 a publication in a scientific journal is planned. You will find the respective link and doi number here, as soon as it got published. The Abstract currently reads:

> This paper documents the design, implementation and evaluation of the Unfolding Space Glove: an open source sensory substitution device that allows blind users to haptically sense the depth of their surrounding space. The prototype requires no external hardware, is highly portable, operates in all lighting conditions, and provides continuous and immediate feedback – all while being visually unobtrusive. Both blind (n = 8) and sighted but blindfolded subjects (n = 6) completed structured training and obstacle courses with the prototype and the white long cane to allow performance comparisons to be drawn between them. Although the subjects quickly learned how to use the glove and successfully completed all of the trials, they could not outperform their results with the white cane within the duration of the study. Nevertheless, the results indicate general processability of spatial information through sensory substitution by means of haptic, vibrotactile interfaces. Moreover, qualitative interviews revealed high levels of usability and user experience with the glove. Further research is necessary to investigate whether performance could be improved through further training, and how a fully functional navigational aid could be derived from this prototype.



## Content and Connected Repos

This repository contains the source code of the Unfolding app, which runs on a Raspberry Pi on the Unfolding Space Glove itself. Other repos containing other parts of the project are:

- **Unfolding Space Hardware**
  Building instructions, blueprints, circuit board files, 3D design files. Without these you cannot run the project reasonably!
- **Monitor**



## Code Documentation

### Make

TODO Tims Makefile beschreiben?

### Run

run with 

````bash
sudo LD_LIBRARY_PATH=/home/dietpi/libroyale/bin ./unfolding-app`
````

add options:

```bash
--help      | show help
--log       | enable general log functions – currently no effect
--printLogs | print log messages in console
--mode arg  | set pico flexx camera mode (int from 0:5)
```

### Overall Code Structure

The main task of the unfolding app is to process the **3D images from the camera as quickly as possible and provide them as a vibration stimulus**. 

The libroyale library (itself in a separate thread) acts as the clock here: the callback function DepthDataListener::onNewData() indicates that a new frame of the camera is ready. This is then immediately copied in order to be able to return onNewData() (requirement of the library). The copied frame is then processed and send to the glove so that a new frames can already be received in this chain before an old one has completely passed through.

Passing the data between the frames involves a lot of locking and thus caution not prevent the code running in e.g. dead locks. Four threads are created in the main loop of the main.cpp – synchonised using condition_variables and notify_one calls.

- **Initialise** all components and **persist in an endless loop** (maintainig time based checks and logs)
- Managing the **UPD connections** and sending values to the monitoring app
- **Copy and Process incoming frame** and pass it to the sending frame
- **Send** the calculated motor values to the glove and **broadcast them via udp**



### Complete Processing Procedure for New Incoming Frame

1. libroyale calls DepthDataListener::onNewData() when a new frame is ready – meaning that it finished its own processes and calculations and provides a royale::DepthData object, containing e.g. depth and confidence calue for each pixel in a two dimensional array.
2. Within onNewData() locks a aquired to not get in conflict with other processes reading the image file at the same time. Then the dataframe is copied to the global Glob::royalDepthData.dat to be accessible to all threads.
3. The processing thread is notified that a ne frame can be processed by using notify_one()
4. In processData(), the processing thread now **analyses the frame and creates a 3x3 matrix of motor values** as a result.
   1. Create some variables:
      - pointer to global frame object
      - an OpenCV matrix to save the image
      - define nine image tiles representing the 3x3 matrix of the output and create depth histograms for each (inside a matrix).
   2. Iterate through all pixels; calc a 0:256 depth value based on the predefined depth range; if measurement confindence (coming from libroyale) is high enough write value to a) the OpenCV matrix and b) the respective histogram of that pixel.
   3. Find the nearest object for each tile/histogram. 
      - Move a sliding window (starting at depth 0, i.e. close to the camera) over all bins of the histogram.
      - check whether or at which depth value the number of pixels in this window exceeds a predefined threshold. 
      - If this is the case, the closest object within this image tile is assumed to be that depth value. Write this value into the global 3x3 Glob::motors.tiles matrix
   4. Notify (notify_one()) the sending thread to transmit the new values to the glove and then send values, image and logs via udp to monitoring app.

And while the process of one frame might still be in point 4, a new frame can already be receiveid via OnNewData(). There is, however, no buffer implemented. If a new frame would arrive before the old one got copied, the old one gets overwritten to avoid any latency.

### UPD Structure / API

To enable the monitoring-app to retrieve values, logs and the depth image, a UDP connection must be established and maintained. 

- The unfolding-app (acting as a server) therefore sends its IP and identification number (important if there are several gloves in a network) to the network once per second. 
- Monitoring-apps (acting as a client) listen to these uptime messages and (if user selects a respective server via GUI) subscribe for information to be send to its IP address.
- The servers maintain a list of subscribed clients and drops those IP addresses if they did not get renewed for one second.

Whenever a new frame is finished processing or sending is invoked in other ways, the server then sends requested information to all subscribed clients. For this asio strands from the boost library are used to create a sequential invocation of send commands by using async_send_to() 

The clients can request different type of information (depending e.g. on use case and network speed) and can control certain behaviour of the unfolding-app by adding information to its subscription message. They can also be combined

- **i |** send full depth image as greyscale image. 
  - when followed by a byte containing 1:9 ascii number: define size of the image (1 being 20*20 pixels only, 9 being the full image)
- **m |**"mute" the vibration motors / disable vibratory output
- **z |** turn certain vibration motor on/off for testing reasons
  - byte containing 1:9 ascii number defines the motor to be switched
- **u |** change camera use case and restart app. Check pico flexx documentation for available use cases (fps and accuracy)
  - byte containing 1:5 ascii number defines the new camera use case. 
- **c |** run calibration process on all motors. Usually we use fixed calibration values to speed up starting time...



## Credits

The project wouldn't have been possible without the help of many people, to whom I would like to express a big thank you at this point:

- **Kjell Wistoff** for his active support in setting up, dismantling and rebuilding the study room, organising the documents and documenting the study photographically.
- Trainer **Regina Beschta** for a free introductory O\&M course and the loan of the study long cane.
- **[a2800276](https://github.com/a2800276)** and **[mattikra](https://github.com/mattikra)** from Press Every Key for their open ear when giving advice on software and hardware.
- **Köln International School of Design/KISD** (TH Köln) and the responsible parties for making the premises available over this long period of time.
- **pmdtechnologies** ag for providing a Pico Flexx camera.
- **Munitec GmbH**, for providing glove samples.
- All those who provided guidance in the development of the prototype over the past years and now in the implementation and evaluation of the study.



## Funding

The 2021-prototype was funded in two ways:

- The study on the device and related expenses were funded as part of my Master's Thesis in an industry-on-campus-cooperation between the University of Tuebingen and Carl Zeiss Vision International GmbH.

- In addition, the prototype construction was funded as part of the Kickstart@TH Cologne project of the StartUpLab@TH Cologne programme (``StartUpLab@TH Cologne'', funding reference 13FH015SU8, Federal Ministry of Education and Research – Germany / BMBF). 

