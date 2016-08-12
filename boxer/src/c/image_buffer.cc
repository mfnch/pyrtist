#include <iostream>
#include <cairo/cairo.h>

#include "image_buffer.h"

ImageBuffer::~ImageBuffer() {
  if (ptr_ != nullptr) {
    delete[] allocated_ptr_;
  }
}

ImageBuffer::ImageBuffer(int width, int height) {
  if (!(width > 0 && height > 0)) {
    allocated_ptr_ = ptr_ = nullptr;
    width_ = height_ = 0;
  }

  size_t sz = width * height;
  ptr_ = allocated_ptr_ = new uint32_t[sz];
  width_ = width;
  height_ = height;
}

void ImageBuffer::Fill(uint32_t value) {
  uint32_t* ptr = ptr_;
  uint32_t* end_ptr = ptr_ + (width_*height_);
  while (ptr < end_ptr)
    *ptr++ = value;
}

bool ImageBuffer::SaveToFile(const char* file_name) {
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
