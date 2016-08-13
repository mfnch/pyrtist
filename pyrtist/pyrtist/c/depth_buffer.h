#ifndef _DEPTH_BUFFER_H
#define _DEPTH_BUFFER_H

#include <limits>
#include <math.h>
#include <stdint.h>

#include "image_buffer.h"

#define LIKELY(x) (x)
#define UNLIKELY(x) (x)

class DepthBuffer : public ImageBuffer<float> {
 public:
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

  DepthBuffer(int width, int height, void* ptr)
      : ImageBuffer(width, width, height, ptr) {
    Clear();
  }

  DepthBuffer(int width, int stride, int height)
      : ImageBuffer(width, stride, height) {
    Clear();
  }

  /// Fill the depth buffer with infinite depth.
  void Clear() { Fill(kInfiniteDepth); }

  ARGBImageBuffer* ComputeNormals();

  void DrawSphere(int x, int y, int z, int radius, float z_scale);
};

#endif /* _DEPTH_BUFFER_H */
