#pragma once

#include "converter.h"
#include "glove.h"
#include "q.h"

#include <royale.hpp>

typedef Q<royale::DepthData *> *DepthQ;

class DepthDataConsumer {
public:
  DepthDataConsumer(DepthQ _q) : q(_q){};
  virtual ~DepthDataConsumer(){};
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
    unsigned char *values;
    while (NULL != (dd = q->retrieve(500))) {
      Converter c(dd);
      values = c.motorMap();
      g->sendValues(values);
      delete (dd);
      delete (values);
    }
  }

private:
  Glove *g;
};