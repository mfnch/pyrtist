#ifndef _IMAGE_BUFFER_H
#define _IMAGE_BUFFER_H

#include <stddef.h>
#include <stdint.h>

class ImageBuffer {
 public:
  ImageBuffer(int width, int height, void* ptr)
      : width_(width), height_(height),
        ptr_(reinterpret_cast<uint32_t*>(ptr)),
        allocated_ptr_(nullptr) { }

  ImageBuffer(int width, int height);

  virtual ~ImageBuffer();

  void* GetPtr() { return ptr_; }

  bool IsValid() {
    return ptr_ != nullptr;
  }

  void Fill(uint32_t value);

  bool SaveToFile(const char* file_name);

 protected:
  int32_t width_,
          height_;
  uint32_t* ptr_;
  uint32_t* allocated_ptr_;
};

#endif /* _IMAGE_BUFFER_H */
