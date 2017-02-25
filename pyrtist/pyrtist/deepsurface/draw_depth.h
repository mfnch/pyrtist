// Copyright (C) 2017 by Matteo Franchin (fnch@users.sf.net)
//
// This file is part of Pyrtist.
//   Pyrtist is free software: you can redistribute it and/or modify it
//   under the terms of the GNU Lesser General Public License as published
//   by the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Pyrtist is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

#ifndef _DRAW_DEPTH_H
#define _DRAW_DEPTH_H

#include <algorithm>
#include <math.h>

#include "image_buffer.h"

template<typename PixelType, typename DepthFn>
void DrawDepth(ImageBuffer<PixelType>* buffer,
               float clip_start_x, float clip_start_y,
               float clip_end_x, float clip_end_y,
               float* matrix, DepthFn depth_fn) {
  if (clip_end_x < clip_start_x) std::swap(clip_start_x, clip_end_x);
  if (clip_end_y < clip_start_y) std::swap(clip_start_y, clip_end_y);
  int ix0 = static_cast<int>(floorf(clip_start_x));
  int iy0 = static_cast<int>(floorf(clip_start_y));
  int ix1 = static_cast<int>(ceilf(clip_end_x));
  int iy1 = static_cast<int>(ceilf(clip_end_y));
  auto region = buffer->GetRegion(ix0, iy0, ix1, iy1);
  if (!region.IsValid()) {
    return;
  }

  float u_at_line_start = matrix[0]*ix0 + matrix[1]*iy0 + matrix[2];
  float v_at_line_start = matrix[3]*ix0 + matrix[4]*iy0 + matrix[5];
  float u_increment_per_pixel = matrix[0];
  float u_increment_per_line = matrix[1];
  float v_increment_per_pixel = matrix[3];
  float v_increment_per_line = matrix[4];

  int32_t width = region.GetWidth();
  int32_t stride = region.GetStride();
  int32_t height = region.GetHeight();
  int32_t skip = stride - width;
  float* ptr = region.GetPtr();
  float* end_ptr = ptr + stride*height;
  PixelType* end_line;
  while ((end_line = ptr + width) < end_ptr) {
    float u = u_at_line_start;
    float v = v_at_line_start;
    while (ptr < end_line) {
      depth_fn(ptr, u, v);
      ptr++;
      u += u_increment_per_pixel;
      v += v_increment_per_pixel;
    }
    ptr += skip;
    u_at_line_start += u_increment_per_line;
    v_at_line_start += v_increment_per_line;
  }
}


#endif  /* _DRAW_DEPTH_H */
