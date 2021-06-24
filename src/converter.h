#pragma once
#include <royale.hpp>

class Converter {
public:
  //Converter(royale::DepthData *_dd) : dd(_dd) {}
  Converter(royale::DepthData *_dd, int _minConfidence = 11,
            float _maxDepth = 1.5, int _minObjSizeThreshold=90)
      : dd(_dd), minConfidence(_minConfidence), maxDepth(_maxDepth), minObjSizeThreshold(_minObjSizeThreshold) {};
  ~Converter() {};

  // it is the responsibility of the caller to dispose of the returned data using free()
  uint8_t * depthImage();
  
  // it is the responsibility of the caller to dispose of the returned data using free()
  uint8_t * motorMap();

private:
  // input depth data to process
  royale::DepthData *dd;
  // the minimum depthConfidence a point in the incoming data needs
  // to have for us to consider it.
  int minConfidence;
  // the maximum depth value we aill consider for incoming data
  float maxDepth;
  // "the min number of pixels, an object must have, (smaller objects might be noise)
  int minObjSizeThreshold;
};