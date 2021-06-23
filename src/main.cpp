
#include <royale.hpp>

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <boost/program_options.hpp>

#include "listener.hpp"

using namespace boost;
using namespace std;

void usage() {
  printf("usage: unfolding\n");
  printf("  [ -l ] list cameras\n");
  printf("  [ -r rrf_file ] load virtual camera from file\n");
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

int main(int argc, char *argv[]) {

  int c;
  char *rrf_filename = NULL;
  while (-1 != (c = getopt(argc, argv, "lr:"))) {
    switch (c) {
    case 'l':
      list_cameras();
      break;
    case 'r':
      rrf_filename = optarg;
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

  // TODO use case options
  Listener listener;

  if (camera->registerDataListener(&listener) !=
      royale::CameraStatus::SUCCESS) {
    printf("couldn't register listener\n");
    exit(1);
  }

  if (camera->startCapture() != royale::CameraStatus::SUCCESS) {
    printf("couldn't start capture\n");
    exit(1);
  }
  printf("will sleep for 5s\n");
  fflush(stdout);
  sleep(5);
  if (camera->stopCapture() != royale::CameraStatus::SUCCESS) {
    printf("couldn't stop capture\n");
    exit(1);
  }
  return 0;
}
