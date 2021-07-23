
#include <royale.hpp>

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <thread>


#include "depthDataConsumer.h"
#include "listener.hpp"

using namespace std;

void usage() {
  printf("usage: unfolding\n");
  printf("  [ -l ] list cameras\n");
  printf("  [ -r rrf_file ] load virtual camera from file\n");
  printf("  [ -s seconds ] run for indicated number of seconds (default: forever)\n");
  exit(1);
}

void list_cameras() {
  royale::CameraManager manager;
  auto cameraList = manager.getConnectedCameraList();
  // TODO check for camera, check rrf error

  if (cameraList.empty()) {
    printf("no camera connected, sorry.\n");
    return;
  }

  for (size_t i = 0; i != cameraList.size(); ++i) {
    // programming like it's 1988! of course libroyale has
    // its own fucking string implementation... Can't possible
    // trust std!
    printf("%ld : %s\n", i, cameraList[i].c_str());
  }
}

void consume(Q<royale::DepthData *> *q) {
  // DepthDataConsumer consumer(q);
  Glove g;
  DepthDataGloveConsumer consumer(q, &g);
  consumer.consume();
}

#ifndef VERSION
#define VERSION "unknown"
// VERSION is defined in the makefile to correspond to the
// git commit ...
#endif

void print_version() { printf("version: %s\n", VERSION); }


int main(int argc, char *argv[]) {

  int c;
  char *rrf_filename = NULL;
  int seconds = 0;
  while (-1 != (c = getopt(argc, argv, "vls:r:"))) {
    switch (c) {
    case 'v':
      print_version();
      break;
    case 'l':
      list_cameras();
      break;
    case 'r':
      rrf_filename = optarg;
      break;
    case 's':
      seconds = atoi(optarg);
      break;
    default:
      usage();
    }
  }

  royale::CameraManager manager;
  std::unique_ptr<royale::ICameraDevice> camera;

  if (rrf_filename != NULL) {
    camera = manager.createCamera(rrf_filename);
  } else {
    // always use first camera if no filename is provided.
    auto cameraList = manager.getConnectedCameraList();
    if (!cameraList.empty()) {
      camera = manager.createCamera(cameraList[0]);
    } else {
      printf("no camera connected, sorry.\n");
      usage();
    }
  }

  if (camera == nullptr) {
    printf("cannot created camera, sorry\n");
    exit(1);
  }

  // init camera
  if (camera->initialize() != royale::CameraStatus::SUCCESS) {
    printf("couldn't init camera\n");
    exit(1);
  }

  // TODO use case options !?
  Q<royale::DepthData *> q;
  Listener listener(&q);
  std::thread thr(consume, &q);

  if (camera->registerDataListener(&listener) !=
      royale::CameraStatus::SUCCESS) {
    printf("couldn't register listener\n");
    exit(1);
  }

  if (camera->startCapture() != royale::CameraStatus::SUCCESS) {
    printf("couldn't start capture\n");
    exit(1);
  }

  if (seconds != 0) {
  	printf("will run for %is\n", seconds);
  	fflush(stdout);
  	sleep(seconds);
  	printf("Stopping Camera\n");
  	if (camera->stopCapture() != royale::CameraStatus::SUCCESS) {
	  printf("couldn't stop capture\n");
	  exit(1);
  	}

  }
  printf("joining thread\n");
  thr.join();
  printf("bye\n");

  return 0;
}
