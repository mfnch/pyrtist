#include <iostream>

#include <algorithm>
#include <math.h>

#include "depth_buffer.h"

#if 0
class DepthFunction {
 private:
};
#endif

void DepthBuffer::DrawSphere(int x, int y, int z, int radius, float z_scale) {
  int begin_y = std::max(0, y - radius),
      end_y   = std::min(height_, y + radius),
      begin_x = std::max(0, x - radius),
      end_x   = std::min(width_, x + radius);
  uint32_t* ptr = &ptr_[width_*begin_y + begin_x];
  uint32_t r2 = radius*radius;
  uint32_t pixels_to_next = width_ - (end_x - begin_x);
  for (int iy = begin_y; iy < end_y; iy++, ptr += pixels_to_next) {
    int dy = iy - y, dy2 = dy*dy;
    for (int ix = begin_x; ix < end_x; ix++, ptr++) {
      int dx = ix - x, dx2 = dx*dx;
      int dz2 = r2 - (dx2 + dy2);
      if (dz2 >= 0) {
        *ptr = int(z_scale*sqrt(dz2));
      }
    }
  }
}

ImageBuffer* DepthBuffer::ComputeNormals() {
  auto normals = new ImageBuffer(width_, height_);
  if (normals == nullptr || width_ < 3 || height_ < 3)
    return normals;
  int32_t* out_ptr = reinterpret_cast<int32_t*>(normals->GetPtr());

  int nx = width_ - 2;
  int ny = height_ - 2;
  int32_t* begin_ptr = reinterpret_cast<int32_t*>(ptr_);
  int32_t* ptr = &begin_ptr[width_];

  for (int iy = 1; iy < ny; iy++) {
    int32_t lft = *ptr++;
    int32_t* top_ptr = ptr - width_;
    int32_t* bot_ptr = ptr + width_;
    int32_t ctr = *ptr++;
    int32_t rgt;
    uint32_t mask = ((lft == kInfiniteDepth ? 1U : 0U) |
                     (ctr == kInfiniteDepth ? 2U : 0U));
    int32_t* final_ptr = ptr + nx;
    while (ptr < final_ptr) {
      rgt = *ptr++;
      mask |= (rgt == kInfiniteDepth ? 4U : 0U);
      if (mask == 0) {
        top_ptr = ptr - width_ - 1;
        bot_ptr = ptr + width_ + 1;

        // The current point has both left and right neighbours.
        int64_t dx = (rgt - lft) >> 1;            // -d/dx depth(x, y)

        // Now compute the y derivative.
        int64_t dy = (*top_ptr - *bot_ptr) >> 1;  // -d/dy depth(x, y)
        int64_t d2 = 1 + dx*dx + dy*dy;
        int64_t d = static_cast<uint64_t>(sqrt(d2));

        uint8_t red   = (dx*127)/d + 128;
        uint8_t green = (dy*127)/d + 128;
        uint8_t blue  = 127/d + 128;
        size_t offset = ptr - begin_ptr;
        out_ptr[offset] = ((static_cast<uint32_t>(red)) |
                           (static_cast<uint32_t>(green) << 8) |
                           (static_cast<uint32_t>(blue) << 16) |
                           (UINT32_C(0xff) << 24));
      } else if (mask == 7) {
        // All empty: ignore this point.
      } else if ((mask & 2) == 0) {
        // Border point: only one neighbour is present.
      }

      lft = ctr;
      ctr = rgt;
      mask >>= 1;
    }
  }

  return normals;
}

void DepthBuffer::DoSomething() {
  size_t sz = width_*height_;
  for (size_t i = 0; i < sz; i++) {
    if (ptr_[i] != 0)
      ptr_[i] |= 0xff000000;
  }
}

int main() {
  DepthBuffer ds(320, 200);
  ds.DrawSphere(160, 100, 0, 80, 3.0);

  auto normals = ds.ComputeNormals();
  normals->SaveToFile("normals.png");
  delete normals;

  ds.DoSomething();
  ds.SaveToFile("test.png");
}
