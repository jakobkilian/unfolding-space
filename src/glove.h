#pragma once

#include <cstdio>

class Glove {
public:
  void sendValues(unsigned char values[]) {
    for (int y = 0; y != 3; ++y) {
      printf("0x%02x 0x%02x 0x%02x\n", values[y * 3], values[y * 3 + 1],
             values[y * 3 + 2]);
    }
    printf("--------------\n");
  }
};