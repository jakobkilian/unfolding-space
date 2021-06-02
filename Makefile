

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
# CXXFLAGS += -std=c++11
# c++11 happens to break boost, and I'm not in the mood to figure out why or why std works. TODO


# depending on how and where we compile, we may want to
# provide the location of opencv during the call to
# make, i.e `$ OPENCVDIR=/some/other/location make`
# `?=` sets this variable only if it hasn't been set.
#
# NOTE TO SELF:
# currently in the xcompile environment call make via:
# CXX=aarch64-linux-gnu-g++ OPENCVDIR=/opencv/arm64 ROYALEDIR=/opt/libroyale WIRINGDIR=/opt/wiringPi make

OPENCVDIR ?= /usr/local
OPENCV_INC_DIR=$(OPENCVDIR)/include

# ditto for libroyale
ROYALEDIR ?= /home/dietpi/libroyale



# Require outside (of project) headers for: opencv, royale and wiring ...
# include flags don't have their own variable used in implicit rules.
CXXFLAGS += -I$(OPENCV_INC_DIR)/opencv4 -I$(ROYALEDIR)/include


# while `make` doesn't differentiate between -I and other compile flags,
# it *does* for `-l` (which indicates which libs to link) and `-L` which indicates
# where to look for libs ... 
LDFLAGS =  -L$(ROYALEDIR)/bin -L$(OPENCVDIR)/lib 
LDLIBS =   -pthread -lroyale -lopencv_core -lopencv_imgproc -lboost_system -lwiringPi

# wiringPi is included in the system path on diet, so for now we'll only fiddle
# with it if it's set externally.
ifdef WIRINGDIR
CXXFLAGS += -I$(WIRINGDIR)/include
LDFLAGS += -L$(WIRINGDIR)/lib
endif

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


 
