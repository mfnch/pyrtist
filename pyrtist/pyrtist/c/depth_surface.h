#ifndef _DEPTH_SURFACE_H
#define _DEPTH_SURFACE_H

#include "image_buffer.h"
#include "depth_buffer.h"

class DepthSurface {
 public:
  DepthSurface(int width, int stride, int height)
      : src_depth_buffer_(width, stride, height),
        src_image_buffer_(width, stride, height),
        dst_depth_buffer_(width, stride, height),
        dst_image_buffer_(width, stride, height) {
    Clear();
  }

  /// Get the depth buffer over which the user is expected to draw.
  DepthBuffer* GetDepthBuffer() { return &src_depth_buffer_; }

  /// Get the image buffer over which the user is expected to draw.
  ARGBImageBuffer* GetImageBuffer() { return &src_image_buffer_; }

  /// Clear the source buffers.
  void Clear() {
    src_depth_buffer_.Clear();
    src_image_buffer_.Clear();
  }

  /// Transfer the source buffer to the main buffers.
  void Transfer(bool and_clear = true);

  bool SaveToFiles(const char* image_file_name, const char* normals_file_name);

 private:
  DepthBuffer src_depth_buffer_;
  ARGBImageBuffer src_image_buffer_;
  DepthBuffer dst_depth_buffer_;
  ARGBImageBuffer dst_image_buffer_;
};

#endif /* _DEPTH_SURFACE_H */
