#include "listener.hpp"

void Listener::onNewData(const royale::DepthData *data) {
  royale::DepthData *mydata = new (royale::DepthData);
  *mydata = *data;
  q->push(mydata);
  // DEBUG
  static int i = 0;
  i++;
  if (i % 10 == 0) {
    printf("pushed: i: %d w: %d h: %d\n", i, mydata->width, mydata->height);
  }
}