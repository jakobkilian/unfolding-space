CC = g++
CCFLAGS = -o3 -O -g -pthread -Wno-psabi -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic
SOURCES = src/MotorBoard.cpp src/Camera.cpp src/TimeLogger.cpp src/UdpServer.cpp src/UdpClient.cpp src/Globals.cpp src/main.cpp 
NAME = unfolding-app
INCFLAGS = -I/usr/local/include/opencv4 -I/home/dietpi/libroyale/include
LDFLAGS =  -L/home/dietpi/libroyale/bin -lroyale -lopencv_core  -lopencv_imgproc -lboost_system -lwiringPi

$(NAME): $(SOURCES)
	$(CC) $(CCFLAGS) -o $(NAME) $(INCFLAGS) $(LDFLAGS) $(SOURCES)
	

	
# Requirements:

# OPENCV
# -I/usr/local/include/opencv4: OpenCV general
# -lopencv_core: OpenCV general
# -lopencv_imgproc: OpenCV, for example cv::resize 

# LIB ROYAL (Pmd Pico Flexx Depth Camera)
# -L/home/dietpi/libroyale/bin 
# -lroyale
# -I/home/dietpi/libroyale/include

# BOOST
# -lboost_system: boost library for sending udp packets

# LWIRING PI
# -lwiringPi: Send Values to glove via i2c


 