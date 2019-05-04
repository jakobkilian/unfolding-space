CC = g++
CCFLAGS = -o3 -O -g
SOURCES = glove.cpp camera.cpp poti.cpp main.cpp
NAME = app
INCFLAGS = -Ilibroyale/include -Ilibroyale/samples/inc `libusb-config --cflags`
LDFLAGS = -Llibroyale/bin -lroyale -lopencv_core -lopencv_highgui -lopencv_imgproc -lwiringPi `libusb-config --libs`

$(NAME): $(SOURCES)
	$(CC) $(CCFLAGS) -o $(NAME) $(INCFLAGS) $(LDFLAGS) $(SOURCES)

clean:
	rm $(NAME)

run: $(NAME)
	LD_LIBRARY_PATH=libroyale/bin ./app $(ARGS)
	
#opencv arguments (not needed anymore) 
