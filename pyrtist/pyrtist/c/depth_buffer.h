#ifndef _DEPTH_BUFFER_H
#define _DEPTH_BUFFER_H

#include <limits>
#include <math.h>
#include <stdint.h>

#include "image_buffer.h"

#define LIKELY(x) (x)
#define UNLIKELY(x) (x)

class DepthBuffer : public ImageBuffer<float> {
 private:
  /// 32-bit type used to store the depth of each pixel.
  using DepthType = PixelType;

  /// DepthType used for pixels where the depth is infinite.
  static constexpr DepthType kInfiniteDepth =
    std::numeric_limits<DepthType>::quiet_NaN();

  /// Check if a depth value is infinite.
  static bool IsInfiniteDepth(DepthType depth) {
    // NOTE: remember that NaN != NaN.
    return isnan(depth);
  }

 public:
  DepthBuffer(int width, int height, void* ptr)
      : ImageBuffer(width, width, height, ptr) {
    Fill(kInfiniteDepth);
  }

  DepthBuffer(int width, int height) : ImageBuffer(width, width, height) {
    Fill(kInfiniteDepth);
  }

  ARGBImageBuffer* ComputeNormals();

  void DrawSphere(int x, int y, int z, int radius, float z_scale);
};

#endif /* _DEPTH_BUFFER_H */
