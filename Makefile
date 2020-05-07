CC = g++
CCFLAGS = -o3 -O -g -pthread
SOURCES = glove.cpp camera.cpp poti.cpp init.cpp main.cpp
NAME = app
INCFLAGS = -I/usr/local/include/opencv4 -I/home/dietpi/libroyale/include `libusb-config --cflags` -I/home/dietpi/libroyale/samples/inc 
LDFLAGS = -L/usr/local/lib -lopencv_shape -lopencv_videoio -L/home/dietpi/libroyale/bin -lroyale  -lopencv_core -lopencv_highgui -lopencv_imgproc -lboost_system -lwiringPi `libusb-config --libs`

$(NAME): $(SOURCES)
	$(CC) $(CCFLAGS) -o $(NAME) $(INCFLAGS) $(LDFLAGS) $(SOURCES)

clean:
	rm $(NAME)

run: $(NAME)
	LD_LIBRARY_PATH=/home/dietpi/libroyale/bin ./app $(ARGS)
	
#opencv arguments (not needed anymore) 
