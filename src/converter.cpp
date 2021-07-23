
#include <cstdlib>
#include <strings.h>

#include "converter.h"

uint8_t *Converter::depthImage() {
  int width = dd->width;
  int height = dd->height;

  uint8_t *ourDepthImage = (uint8_t *)calloc(sizeof(uint8_t), width * height);

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

// This is my understanding of the algorithm to map the DepthData onto
// a 9x9 grid of motors, each of which can vibrate in 256 intensity levels:
// High Level Overview:
//   - map each depth data value, up to the max distance we are
//     interested in, to values between 0..255
//   - divide the depth data "picture" into 9 fields, each corresponding
//     spatially to one of the motors
//   - identify the closest object in each of the fields and vibrate the
//     corresponding motor in a corresponding intensity
//
// Low Level Description:
//
// - visit each pixel (DepthPoint) of the depth image
// - if the `depthConfidence` value does not exceed our predefined threshold
//   mark it invalid ( 0xff )
// - if the `z` value of the DepthPoint is greater than `maxDepth` mark it
//   invalid
// - else, map the `z` value (0.0 ... maxDepth) to the range 0x00..0xFF [1]
//
// - create 9 histograms, each corresponding to one of the outputs (i.e. motors)
// - the histograms have 256 bins, one for each of the possible discrete pixel
//   values resulting from the mapping. Each bin contains the number
//   of pixels with the corresponding value contained in the area of the depth
//   image corresponding to that histogram's output. [2]
//
// - once the histograms are created, we visit each one and:
// - ignore the first 17 bins. This value resulted in informal observation that
//   these values tend to fluctuate strongly. [3]
// - create a sliding window (size `range=50`) over the bins and calculate the
//   sum of all the pixel counts in the window. Pixel counts < 5 are not
//   included in the sum. [4]
// - the first bin of the first window for which the sum exceeds
//   `minObjSizeThreshold` is understood to be the value of the closest object
//   visible in the areas of the depth image corresponding to that histogram /
//   output / motor.
//
// [1] we could/should use a larger data type than uint8 in order to be able to
//     explicitly mark low confidence and out of range values (or use a lower
//     output resolution than 255)
//     Suggestion: [map to: min..max to 0.0...1.0 INF=out-of-range NaN=invalid]
// [2] in the original version each bin was a uint8, but this would overflow:
//     the depth picture is 171 x 224 ~ 38k pixels. If each pixel had the
//     identical value, the corresponding histogram bin would have a count of
//     4256
//     [=(171x224)/9  > 255]
// [3] Todo : this value should be given a name and made configurable.
// [4] I forgot the rationale behind this, but the parameter shoudld at least be
//     named and configurable. We had some discussion of whether a large number
//     of consecutive bins containing 0 (or less than 5) pixels should be
//     disregarded ...
uint8_t *Converter::motorMap() {

  // histogram is actually a 9x9 grid of histograms, each with 255 bins.
  short histogram[9][256] = {0};
  uint8_t *map = (uint8_t *)calloc(sizeof(uint8_t), 9);

  int width = dd->width;         // get width from depth image
  int height = dd->height;       // get height from depth image
  int tileWidth = width / 3 + 1; // respective width of one tile
  int tileHeight = height / 3 + 1;
  // ?! this doesn't seem right, a 9x9 input image should have 3x3 bins ...

  for (int y = 0; y != height; ++y) {
    for (int x = 0; x != width; ++x) {
      auto depthPoint = dd->points.at(y * height + x);
      uint8_t histogram_depth = 255;

      if (depthPoint.depthConfidence >= minConfidence &&
          depthPoint.z <= maxDepth && maxDepth > 0) {
        histogram_depth = (uint8_t)depthPoint.z / maxDepth * 255.0f;
      }
      int histogram_tile_idx = (3 * (y / tileHeight)) + (x / tileWidth);
      //printf("hdepth %i z:%i\n", histogram_depth, depthPoint.z);
      histogram[histogram_tile_idx][histogram_depth] += 1;
    } // for x
  }   // for y

  for (int tile = 0; tile != 9; ++tile) {
    const int first_bin = 17;
    // exclude the first 17cm (bins) because of oversaturation
    // issues and noisy data the Pico Flexx has in this range
    const int window_size = 50; //"look in tolerance range of 50cm"

    int distance_bin_number =
        first_bin; // bin number corresponds to distance 0x00..0x255
    int sum = 0;
    for (; distance_bin_number != (first_bin + window_size);
         ++distance_bin_number) {
      // calculate the sum of the first window.
      short num_pixels = histogram[tile][distance_bin_number];
      sum += num_pixels > 5 ? num_pixels : 0;
    }

    for (; distance_bin_number < 256; ++distance_bin_number) {
      if (sum >= minObjSizeThreshold) {
	      printf("here: %i\n", sum);
        break;
      }
      // update the sliding window.
      sum += histogram[tile][distance_bin_number] > 5
                 ? histogram[tile][distance_bin_number]
                 : 0;
      sum -= histogram[tile][distance_bin_number - window_size] > 5
                 ? histogram[tile][distance_bin_number - window_size]
                 : 0;
      printf("sunm: %i\n", sum);
    }
    // we've either exceeded minObjSizeThreshold and 'break'ed out of the `for`
    // loop or reached the last bin.
    map[tile] = distance_bin_number != 256 ? distance_bin_number : 255;
  }
  return map;
}
