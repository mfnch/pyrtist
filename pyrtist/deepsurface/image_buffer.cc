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

#include <iostream>
#include <cairo/cairo.h>

#include "image_buffer.h"

bool ARGBImageBuffer::SaveToFile(const char* file_name) {
  bool success = false;
  cairo_format_t fmt = CAIRO_FORMAT_ARGB32;
  unsigned char *data = reinterpret_cast<unsigned char*>(ptr_);
  int stride = cairo_format_stride_for_width(fmt, width_);
  if (stride == 4*width_) {
    cairo_surface_t* cs =
      cairo_image_surface_create_for_data(data, fmt, width_, height_, stride);
    success =
      (cairo_surface_status(cs) == CAIRO_STATUS_SUCCESS &&
       cairo_surface_write_to_png(cs, file_name) == CAIRO_STATUS_SUCCESS);
    cairo_surface_destroy(cs);
  }
  return success;
}

void ARGBImageBuffer::DrawCircle(int x, int y, int radius, uint32_t color) {
  int begin_y = std::max(0, y - radius),
      end_y   = std::min(height_, y + radius),
      begin_x = std::max(0, x - radius),
      end_x   = std::min(width_, x + radius);
  PixelType* begin_ptr = GetPtr();
  PixelType* ptr = &begin_ptr[width_*begin_y + begin_x];
  uint32_t r2 = radius*radius;
  uint32_t pixels_to_next = width_ - (end_x - begin_x);
  for (int iy = begin_y; iy < end_y; iy++, ptr += pixels_to_next) {
    int dy = iy - y, dy2 = dy*dy;
    for (int ix = begin_x; ix < end_x; ix++, ptr++) {
      int dx = ix - x, dx2 = dx*dx;
      int dz2 = r2 - (dx2 + dy2);
      if (dz2 >= 0)
        *ptr = color;
    }
  }
}
