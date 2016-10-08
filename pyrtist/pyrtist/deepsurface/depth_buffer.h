#ifndef _DEPTH_BUFFER_H
#define _DEPTH_BUFFER_H

#include <limits>
#include <functional>
#include <memory>
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

  bool SaveToFile(const char* file_name) {
    if (file_name == nullptr)
      return false;
    std::unique_ptr<ARGBImageBuffer> normals_image{ComputeNormals()};
    return normals_image->SaveToFile(file_name);
  }

  void DrawPlane(float clip_start_x, float clip_start_y,
                 float clip_end_x, float clip_end_y,
                 float* mx, float z00, float z10, float z01);

  void DrawStep(float clip_start_x, float clip_start_y,
                float clip_end_x, float clip_end_y,
                float start_x, float start_y, float start_z,
                float end_x, float end_y, float end_z);

  void DrawSphere(float clip_start_x, float clip_start_y,
                  float clip_end_x, float clip_end_y,
                  float* mx, float z_start, float z_end);

  void DrawCylinder(float clip_start_x, float clip_start_y,
                    float clip_end_x, float clip_end_y,
                    float* mx, float end_radius,
                    float z_of_axis, float start_radius_z, float end_radius_z);

  void DrawCircular(float clip_start_x, float clip_start_y,
                    float clip_end_x, float clip_end_y,
                    float* mx, float scale_z, float translate_z,
                    std::function<float(float)> radius_fn);
};

#endif  /* _DEPTH_BUFFER_H */
