#include <iostream>

#include <algorithm>
#include <math.h>

#include "deep_buffer.h"

void DeepBuffer::DrawStep(float clip_start_x, float clip_start_y,
                          float clip_end_x, float clip_end_y,
                          float start_x, float start_y, float start_z,
                          float end_x, float end_y, float end_z) {
  if (clip_end_x < clip_start_x) std::swap(clip_start_x, clip_end_x);
  if (clip_end_y < clip_start_y) std::swap(clip_start_y, clip_end_y);
  int ix0 = static_cast<int>(floorf(clip_start_x));
  int iy0 = static_cast<int>(floorf(clip_start_y));
  int ix1 = static_cast<int>(ceilf(clip_end_x));
  int iy1 = static_cast<int>(ceilf(clip_end_y));
  auto region = GetRegion(ix0, iy0, ix1, iy1);
  if (!region.IsValid()) {
    std::cout << "NOT VALID" << std::endl;
    return;
  }

  float min_z = std::min(start_z, end_z);
  float max_z = std::max(start_z, end_z);
  float dx = end_x - start_x;
  float dy = end_y - start_y;
  float dz = end_z - start_z;
  float v2 = dx*dx + dy*dy;
  float increment_x = dx*dz/v2;
  float increment_y = dy*dz/v2;
  float z_at_line_start = (((start_z*end_x - end_z*start_x)*dx +
                            (start_z*end_y - end_z*start_y)*dy)/v2 +
                           increment_x*ix0 + increment_y*iy0);

  int32_t width = region.GetWidth();
  int32_t stride = region.GetStride();
  int32_t height = region.GetHeight();
  int32_t skip = stride - width;
  float* ptr = region.GetPtr();
  float* end_ptr = ptr + stride*height;
  PixelType* end_line;
  while ((end_line = ptr + width) < end_ptr) {
    float z = z_at_line_start;
    while (ptr < end_line) {
      if (z >= min_z && z < max_z)
        *ptr = z;
      ptr++;
      z += increment_x;
    }
    ptr += skip;
    z_at_line_start += increment_y;
  }
}

void DeepBuffer::DrawSphere(int x, int y, int z, int radius, float z_scale) {
  int begin_y = std::max(0, y - radius),
      end_y   = std::min(height_, y + radius),
      begin_x = std::max(0, x - radius),
      end_x   = std::min(width_, x + radius);
  DepthType* begin_ptr = GetPtr();
  DepthType* ptr = &begin_ptr[width_*begin_y + begin_x];
  uint32_t r2 = radius*radius;
  uint32_t pixels_to_next = width_ - (end_x - begin_x);
  for (int iy = begin_y; iy < end_y; iy++, ptr += pixels_to_next) {
    int dy = iy - y, dy2 = dy*dy;
    for (int ix = begin_x; ix < end_x; ix++, ptr++) {
      int dx = ix - x, dx2 = dx*dx;
      int dz2 = r2 - (dx2 + dy2);
      if (dz2 >= 0)
        *ptr = DepthType(z_scale*sqrt(dz2) + z);
    }
  }
}

ARGBImageBuffer* DeepBuffer::ComputeNormals() {
  auto normals = new ARGBImageBuffer(width_, width_, height_);
  if (normals == nullptr || width_ < 3 || height_ < 3)
    return normals;
  int32_t* out_ptr = reinterpret_cast<int32_t*>(normals->GetPtr());

  int nx = width_ - 2;
  int ny = height_ - 2;
  DepthType* begin_ptr = GetPtr();
  DepthType* ptr = &begin_ptr[width_];

  for (int iy = 1; iy < ny; iy++) {
    DepthType lft = *ptr++;
    DepthType* top_ptr = ptr - width_;
    DepthType* bot_ptr = ptr + width_;
    DepthType ctr = *ptr++;
    DepthType rgt;
    uint32_t x_mask = ((IsInfiniteDepth(lft) ? 1U : 0U) |
                       (IsInfiniteDepth(ctr) ? 2U : 0U));
    DepthType* final_ptr = ptr + nx;
    while (ptr < final_ptr) {
      size_t offset = ptr - 1 - begin_ptr;
      rgt = *ptr++;
      x_mask |= (IsInfiniteDepth(rgt) ? 4U : 0U);

      if ((x_mask & 2U) != 0) {
        // Point with infinite depth: use default colors.
        out_ptr[offset] = 0xff8080ff;
      } else {
        top_ptr = ptr - 2 - width_;
        bot_ptr = ptr - 2 + width_;
        DepthType top = *top_ptr;
        DepthType bot = *bot_ptr;
        bool top_absent = IsInfiniteDepth(top);
        bool bot_absent = IsInfiniteDepth(bot);

        float dy;  // Compute -d/dy depth(x, y) in dy.
        if (LIKELY(top_absent == bot_absent))
          dy = UNLIKELY(top_absent) ? 0.0f : (bot - top)*0.5f;
        else
          dy = top_absent ? bot - ctr : ctr - top;

        float dx;  // Compute -d/dx depth(x, y) in dx.
        if (LIKELY(x_mask == 0U))
          dx = (rgt - lft)*0.5f;
        else if (x_mask == 4U)
          dx = ctr - lft;
        else if (x_mask == 1U)
          dx = rgt - ctr;
        else
          dx = 0.0f;

        // Now compute the norm of the unnormalized normal vector.
        float d = sqrtf(1.0f + dx*dx + dy*dy);

        uint8_t red   = 128 + static_cast<uint8_t>((dx*127.0f)/d);
        uint8_t green = 128 + static_cast<uint8_t>((dy*127.0f)/d);
        uint8_t blue  = 128 + static_cast<uint8_t>(127.0/d);
        out_ptr[offset] = ((static_cast<uint32_t>(blue)) |
                           (static_cast<uint32_t>(green) << 8) |
                           (static_cast<uint32_t>(red) << 16) |
                           (UINT32_C(0xff) << 24));
      }

      lft = ctr;
      ctr = rgt;
      x_mask >>= 1;
    }
  }

  return normals;
}
