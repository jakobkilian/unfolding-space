#pragma once

#include "glove.h"
#include "q.h"

#include <royale.hpp>

typedef Q<royale::DepthData *> *DepthQ;

class DepthDataConsumer {
public:
  DepthDataConsumer(DepthQ _q) : q(_q){};
  virtual ~DepthDataConsumer() {};
  virtual void consume() {

    royale::DepthData *dd;
    while (NULL != (dd = q->retrieve(500))) {
      delete (dd);

      // DEBUG
      static int i = 0;
      i++;
      if (i % 10 == 0) {
        printf("consumed: i: %d w: %d h: %d\n", i, dd->width, dd->height);
      }
    }
  }

protected:
  DepthQ q;
};

class DepthDataGloveConsumer : DepthDataConsumer {

public:
  DepthDataGloveConsumer(DepthQ _q, Glove *_g) : DepthDataConsumer(_q), g(_g) {}
  void consume() override {
    royale::DepthData *dd;
    unsigned char values[9];
    while (NULL != (dd = q->retrieve(500))) {
      values[0] = 0;
      values[1] = 1;
      values[2] = dd->width & 0xff;
      values[3] = dd->height & 0xff;
      values[4] = 4;
      values[5] = 5;
      values[6] = 6;
      values[7] = 7;
      values[8] = 8;
      g->sendValues(values);
      delete (dd);
    }
  }

private:
  Glove *g;
};