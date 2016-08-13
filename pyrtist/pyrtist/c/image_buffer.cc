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
    success = (cairo_surface_status(cs) == CAIRO_STATUS_SUCCESS &&
               cairo_surface_write_to_png(cs, file_name));
    cairo_surface_destroy(cs);
  }
  return success;
}
