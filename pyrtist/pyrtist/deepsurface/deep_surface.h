#ifndef _DEEP_SURFACE_H
#define _DEEP_SURFACE_H

#include "image_buffer.h"
#include "depth_buffer.h"

class DeepSurface {
 public:
  /// Transfer the surface to the given destination surface.
  static bool Transfer(DepthBuffer* src_depth, ARGBImageBuffer* src_image,
                       DepthBuffer* dst_depth, ARGBImageBuffer* dst_image);

  static bool Transfer(DeepSurface* src, DeepSurface* dst) {
    return Transfer(&src->depth_buffer_, &src->image_buffer_,
                    &dst->depth_buffer_, &dst->image_buffer_);
  }

  DeepSurface(int width, int stride, int height)
      : depth_buffer_(width, stride, height),
        image_buffer_(width, stride, height) {
    if (IsValid())
      Clear();
  }

  /// Whether the surface is valid and ready to be used.
  bool IsValid() { return depth_buffer_.IsValid() && image_buffer_.IsValid(); }

  DepthBuffer* GetDepthBuffer() { return &depth_buffer_; }
  ARGBImageBuffer* GetImageBuffer() { return &image_buffer_; }

  void Clear() {
    depth_buffer_.Clear();
    image_buffer_.Clear();
  }

  bool SaveToFiles(const char* image_file_name,
                   const char* normals_file_name) {
    bool success = true;
    if (image_file_name != nullptr)
      success = image_buffer_.SaveToFile(image_file_name);
    if (normals_file_name)
      success = depth_buffer_.SaveToFile(normals_file_name) && success;
    return success;
  }

 private:
  DepthBuffer depth_buffer_;
  ARGBImageBuffer image_buffer_;
};

#endif /* _DEEP_SURFACE_H */
