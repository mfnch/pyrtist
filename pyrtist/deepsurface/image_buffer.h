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

#ifndef _IMAGE_BUFFER_H
#define _IMAGE_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <algorithm>
#include <memory>

template <typename T>
class ImageBuffer {
 public:
  using PixelType = T;

  static ImageBuffer<T> BuildInvalid() {
    return ImageBuffer<T>(0, 0, 0, nullptr);
  }

  ImageBuffer(int width, int stride, int height, void* ptr)
      : ptr_(reinterpret_cast<PixelType*>(ptr)), allocated_ptr_(nullptr),
        width_(width), stride_(stride), height_(height) { }

  ImageBuffer(int width, int stride, int height) {
    if (width > 0 && height > 0 && stride >= width) {
      size_t sz = stride*height;
      ptr_ = allocated_ptr_ = new PixelType[sz];
      width_ = width;
      stride_ = stride;
      height_ = height;
    } else {
      allocated_ptr_ = ptr_ = nullptr;
      width_ = stride_ = height_ = 0;
    }
  }

  virtual ~ImageBuffer() {
    if (allocated_ptr_ != nullptr) {
      delete[] allocated_ptr_;
    }
  }

  int GetWidth() { return width_; }
  int GetStride() { return stride_; }
  int GetHeight() { return height_; }
  PixelType* GetPtr() { return ptr_; }
  size_t GetSizeInBytes() { return height_*stride_*sizeof(PixelType); }

  /// @brief Call the given function for every pixel, passing the pixel pointer.
  void Map(std::function<void(PixelType*)> fn) {
    PixelType* end_ptr = ptr_ + stride_*height_;
    int32_t skip = stride_ - width_;
    PixelType* end_line;
    PixelType* ptr = ptr_;
    while ((end_line = ptr + width_) < end_ptr) {
      while (ptr < end_line)
        fn(ptr++);
      ptr += skip;
    }
  }

  /// @brief Fill the buffer with a uniform value.
  void Fill(PixelType value) {
    Map([value](PixelType* pixel)->void { *pixel = value; });
  }

  /// Return a region of the buffer as a new buffer.
  ImageBuffer<T> GetRegion(int start_x, int start_y, int end_x, int end_y) {
    if (start_x >= end_x) std::swap(start_x, end_x);
    if (start_y >= end_y) std::swap(start_y, end_y);
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (end_x > width_) end_x = width_;
    if (end_y > height_) end_y = height_;
    int32_t width = end_x - start_x;
    int32_t height = end_y - start_y;
    if (width <= 0 || height <= 0)
      return BuildInvalid();
    size_t offset = (static_cast<size_t>(start_y)*static_cast<size_t>(stride_)
                     + static_cast<size_t>(start_x));
    return ImageBuffer<T>(width, stride_, height, &ptr_[offset]);
  }

  /// Whether the buffer is valid and can be used.
  bool IsValid() {
    return (ptr_ != nullptr &&
            width_ > 0 && height_ > 0 && width_ <= stride_);
  }

 protected:
  PixelType* ptr_;
  PixelType* allocated_ptr_;
  int32_t width_;
  int32_t stride_;
  int32_t height_;
};

// Utility functions (Little-endian implementation).
constexpr uint32_t GetA(uint32_t argb) { return (argb >> 24) & 0xff; }
constexpr uint32_t GetR(uint32_t argb) { return (argb >> 16) & 0xff; }
constexpr uint32_t GetG(uint32_t argb) { return (argb >> 8) & 0xff; }
constexpr uint32_t GetB(uint32_t argb) { return argb & 0xff; }
constexpr uint32_t GetARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
  return ((static_cast<uint32_t>(a) << 24) |
          (static_cast<uint32_t>(r) << 16) |
          (static_cast<uint32_t>(g) << 8) |
          (static_cast<uint32_t>(b)));
}

// Blending between premultiplied ARGB values.
// Premultiplied means that R, G, B are intended as already multiplied by the
// corresponding A value, which is the format we use here.
inline uint32_t BlendSrcOverDst(uint32_t src_argb, uint32_t dst_argb) {
  uint32_t src_a = GetA(src_argb);
  if (src_a == 0xff)
    return src_argb;
  uint32_t src_am1 = UINT32_C(0xff) - src_a;
  uint32_t out_a = src_a          + (GetA(dst_argb)*src_am1)/UINT32_C(0xff);
  uint32_t out_r = GetR(src_argb) + (GetR(dst_argb)*src_am1)/UINT32_C(0xff);
  uint32_t out_g = GetG(src_argb) + (GetG(dst_argb)*src_am1)/UINT32_C(0xff);
  uint32_t out_b = GetB(src_argb) + (GetB(dst_argb)*src_am1)/UINT32_C(0xff);
  return GetARGB(out_a, out_r, out_g, out_b);
}

class ARGBImageBuffer : public ImageBuffer<uint32_t> {
 private:
  using BaseType = ImageBuffer<uint32_t>;

 public:
  using BaseType::BaseType;

  /// Clear the content of the buffer.
  void Clear() { Fill(0U); }

  void DrawCircle(int x, int y, int radius, uint32_t color);

  bool SaveToFile(const char* file_name);
};

#endif  /* _IMAGE_BUFFER_H */
