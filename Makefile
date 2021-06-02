

# warning: these are a bit arbitrary, but ... 
#   also switched to CXXFLAGS because we are using C++ (careful: CPPFLAGS are for the preprocessor ...)
CXXFLAGS = -Wall -Wextra -pedantic -Wno-psabi -Wshadow -Wnon-virtual-dtor

# Turn on debug flag for compilation, we may get to this at some later point and differentiate 
# between DEBUG and RELEASE builds
CXXFLAGS += -g

# make the compiler aware we are using pthreads
CXXFLAGS += -pthread

# These are almost certainly wrong of confused. -O sets the optimization level and and -o
# sets the output, so I imagine something got switched around...
# CXXFLAGS += -o3 -O 
CXXFLAGS += -O3
# pick a standard ...
CXXFLAGS += -std=c++11

# Require outside (of project) headers for: opencv, royale and wiring ...
# include flags don't have their own variable used in implicit rules.
CXXFLAGS += -I/usr/local/include/opencv4 -I/home/dietpi/libroyale/include

# while `make` doesn't differentiate between -I and other compile flags,
# it does for `-l` (which indicates which libs to link) and `-L` which indicates
# where to look for libs ... 
LDFLAGS =  -L/home/dietpi/libroyale/bin 
LDLIBS =   -pthread -lroyale -lopencv_core  -lopencv_imgproc -lboost_system -lwiringPi

# don't need to do this manually, we'll just forget if we ever add a file
# SOURCES = src/MotorBoard.cpp src/Camera.cpp src/TimeLogger.cpp src/UdpServer.cpp src/UdpClient.cpp src/Globals.cpp src/main.cpp 

SOURCES = $(wildcard src/*.cpp)
OBJS    = $(patsubst %.cpp, %.o, $(SOURCES))

NAME = unfolding-app

$(NAME) : $(OBJS)
	$(CXX) $(LDFLAGS) $(LDLIBS) -o $@ $^

.PHONY : clean

clean:
	$(RM) $(OBJS)


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


 
