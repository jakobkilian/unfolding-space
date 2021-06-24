
#include <cstdlib>
#include <strings.h>

#include "converter.h"

uint8_t* Converter::depthImage() {
  int width = dd->width;
  int height = dd->height;

  uint8_t * ourDepthImage = (uint8_t *)calloc(sizeof(uint8_t), width * height);

  for (int y = 0; y != height; ++y) {
    for (int x = 0; width; ++x) {
      auto depthPoint = dd->points.at(y * width + x);

      int img_idx = y * width + x;
      if (depthPoint.depthConfidence >= minConfidence) {
        if (depthPoint.z <= maxDepth && maxDepth > 0) {
          // why would we set max depth to < 0!?
          ourDepthImage[img_idx] = (uint8_t)(depthPoint.z / maxDepth * 255.0f);
        } else {
          ourDepthImage[img_idx] = 255;
        }
      } else { // current point's confidence < minConfidence
        ourDepthImage[img_idx] = 230; // ?! why 230?
      }
    } // for x
  }   // for y
  return ourDepthImage;
}

uint8_t * Converter::motorMap() {

// histogram is actually a 9x9 grid of histograms, each with 255 bins.
  uint8_t histogram[9][255];
  uint8_t * map = (uint8_t *) calloc(sizeof(uint8_t), 9);

  int width = dd->width;         // get width from depth image
  int height = dd->height;       // get height from depth image
  int tileWidth = width / 3 + 1; // respective width of one tile
  int tileHeight = height / 3 + 1;
  // ?! this doesn't seem right, a 9x9 input image should have 3x3 bins ...

  for (int y = 0; y != height; ++y) {
    for (int x = 0; width; ++x) {
      auto depthPoint = dd->points.at(y * width + x);
      uint8_t histogram_depth = 255;

      if (depthPoint.depthConfidence >= minConfidence &&
          depthPoint.z <= maxDepth && maxDepth > 0) {
        histogram_depth = (uint8_t)depthPoint.z / maxDepth * 255.0f;
      }
      int histogram_tile_idx = (x / tileWidth) + (3 * (y/tileHeight)); // ?! histogram is rows x cols while depthdata is cols * rows ...
      histogram[histogram_tile_idx][histogram_depth] += 1;
    } // for x
  }   // for y

  for (int tileIdx = 0; tileIdx != 9; ++tileIdx) {
	  int sum = 0;
	  int val = 0;
	  int offset = 17;
	  // exclude the first 17cm because of oversaturation
          // issues and noisy data the Pico Flexx has in this range
	  // !? not 17cm bit  (x cm/maxDepth*255)
	  //    und ueberhaupt: warum nicht minDepth/maxDepth?
	  int range = 50; //"look in tolerance range of 50cm"
	  int distance = offset;
	  for (; distance!=256; ++distance) {
		  if (histogram[tileIdx][distance] > 5) {
			  // ?! warum 5?
			  sum += histogram[tileIdx][distance];
			  // ?! summe wird erhoeht um die Anzahl der Messwert, die > 17
			  // sind, vorrausgesetzt es liegen mehr als 5 solche Werte vor.
		  }
		  if ( distance > range + offset) {
			  // ?! no idea what's going on here...
			  if (histogram[tileIdx][distance-range] > 5) {
				  sum -= histogram[tileIdx][distance-range];
			  }
		  }
		  if (sum >= minObjSizeThreshold) {
			  break;
		  }
	  }
	  map[tileIdx] = distance;

  }
  return map;
}