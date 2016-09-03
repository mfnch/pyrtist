#ifndef _IMAGE_BUFFER_H
#define _IMAGE_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <algorithm>

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

  /// Fill the buffer with a uniform value.
  void Fill(PixelType value) {
    PixelType* end_ptr = ptr_ + stride_*height_;
    int32_t skip = stride_ - width_;
    PixelType* end_line;
    PixelType* ptr = ptr_;
    while ((end_line = ptr + width_) < end_ptr) {
      while (ptr < end_line)
        *ptr++ = value;
      ptr += skip;
    }
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

#endif /* _IMAGE_BUFFER_H */
