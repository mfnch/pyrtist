#ifndef _DEPTH_BUFFER_H
#define _DEPTH_BUFFER_H

#include <stdint.h>

#include "image_buffer.h"

class DepthBuffer : public ImageBuffer {
 private:
  static constexpr int32_t kInfiniteDepth = INT32_C(0x80000000);

 public:
  DepthBuffer(int width, int height, void* ptr)
      : ImageBuffer(width, height, ptr) {
    Fill(kInfiniteDepth);
  }

  DepthBuffer(int width, int height) : ImageBuffer(width, height) {
    Fill(kInfiniteDepth);
  }

  ImageBuffer* ComputeNormals();

  void DrawSphere(int x, int y, int z, int radius, float z_scale);

  void DoSomething();
};

#endif /* _DEPTH_BUFFER_H */
