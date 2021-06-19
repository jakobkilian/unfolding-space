
#include <royale.hpp>

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <boost/program_options.hpp>

#include "listener.hpp"

using namespace boost;
using namespace std;

int main(int argc, char* argv[]) {
	royale::CameraManager manager;
	auto cameraList = manager.getConnectedCameraList();
	// TODO check for camera, check rrf error
	
	if (cameraList.empty()) {
		printf("no camera connected, sorry.\n");
		exit(1);
	}

	
	// always use first camera
	auto camera = manager.createCamera(cameraList[0]);
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

	if (camera->registerDataListener(&listener) != royale::CameraStatus::SUCCESS) {
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