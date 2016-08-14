#ifndef _DEEP_SURFACE_H
#define _DEEP_SURFACE_H

#include "image_buffer.h"
#include "deep_buffer.h"

class DeepSurface {
 public:
  DeepSurface(int width, int stride, int height)
      : src_deep_buffer_(width, stride, height),
        src_image_buffer_(width, stride, height),
        dst_deep_buffer_(width, stride, height),
        dst_image_buffer_(width, stride, height) {
    if (IsValid())
      Clear();
  }

  /// Whether the surface is valid and ready to be used.
  bool IsValid() {
    return (src_deep_buffer_.IsValid() &&
            src_image_buffer_.IsValid() &&
            dst_deep_buffer_.IsValid() &&
            dst_image_buffer_.IsValid());
  }

  /// Get the depth buffer over which the user is expected to draw.
  DeepBuffer* GetDeepBuffer() { return &src_deep_buffer_; }

  /// Get the image buffer over which the user is expected to draw.
  ARGBImageBuffer* GetImageBuffer() { return &src_image_buffer_; }

  /// Clear the source buffers.
  void Clear() {
    src_deep_buffer_.Clear();
    src_image_buffer_.Clear();
  }

  /// Transfer the source buffer to the main buffers.
  void Transfer(bool and_clear = true);

  bool SaveToFiles(const char* image_file_name, const char* normals_file_name);

 private:
  DeepBuffer src_deep_buffer_;
  ARGBImageBuffer src_image_buffer_;
  DeepBuffer dst_deep_buffer_;
  ARGBImageBuffer dst_image_buffer_;
};

#endif /* _DEEP_SURFACE_H */
