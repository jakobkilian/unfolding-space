#include "listener.hpp"

void Listener::onNewData(const royale::DepthData * data) {
	this->i++;
	royale::DepthData * mydata = new(royale::DepthData);
	*mydata = *data;
	if (this->i % 10 == 0) {
		printf("i: %d w: %d h: %d\n", this->i, mydata->width, mydata->height);
	}
	delete(mydata);
}