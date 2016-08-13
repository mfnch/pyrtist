#ifndef _IMAGE_BUFFER_H
#define _IMAGE_BUFFER_H

#include <stddef.h>
#include <stdint.h>

template <typename T>
class ImageBuffer {
 public:
  using PixelType = T;

  ImageBuffer(int width, int stride, int height, void* ptr)
      : ptr_(reinterpret_cast<PixelType*>(ptr)), allocated_ptr_(nullptr),
        width_(width), stride_(stride), height_(height) { }

  ImageBuffer(int width, int stride, int height) {
    if (width > 0 && height > 0 && stride >= width) {
      size_t sz = stride * height;
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
    if (ptr_ != nullptr) {
      delete[] allocated_ptr_;
    }
  }

  int GetWidth() { return width_; }
  int GetStride() { return stride_; }
  int GetHeight() { return height_; }
  PixelType* GetPtr() { return ptr_; }

  void Fill(PixelType value) {
    PixelType* ptr = ptr_;
    PixelType* end_ptr = ptr_ + (width_*height_);
    while (ptr < end_ptr)
      *ptr++ = value;
  }

  bool IsValid() { return ptr_ != nullptr; }

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
