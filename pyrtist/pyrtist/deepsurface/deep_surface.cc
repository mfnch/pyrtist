#include <assert.h>

#include "deep_surface.h"

// Utility functions.
static uint32_t GetA(uint32_t argb) { return (argb >> 24) & 0xff; }
static uint32_t GetR(uint32_t argb) { return (argb >> 16) & 0xff; }
static uint32_t GetG(uint32_t argb) { return (argb >> 8) & 0xff; }
static uint32_t GetB(uint32_t argb) { return argb & 0xff; }
static uint32_t GetARGB(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
  return ((static_cast<uint32_t>(a) << 24) |
          (static_cast<uint32_t>(r) << 16) |
          (static_cast<uint32_t>(g) << 8) |
          (static_cast<uint32_t>(b)));
}

static uint32_t BlendSrcOverDst(uint32_t src_argb, uint32_t dst_argb) {
  uint32_t src_a = GetA(src_argb);
  uint32_t dst_a = GetA(dst_argb);

  uint32_t src_am1 = UINT32_C(0xff) - src_a;
  uint32_t src_weight = src_a;
  uint32_t dst_weight = (dst_a*src_am1)/UINT32_C(0xff);
  uint32_t out_a = src_weight + dst_weight;
  uint32_t out_r = (GetR(src_argb)*src_weight + GetR(dst_argb)*dst_weight)/out_a;
  uint32_t out_g = (GetG(src_argb)*src_weight + GetG(dst_argb)*dst_weight)/out_a;
  uint32_t out_b = (GetB(src_argb)*src_weight + GetB(dst_argb)*dst_weight)/out_a;
  return GetARGB(out_a, out_r, out_g, out_b);
}

bool DeepSurface::SaveToFiles(const char* image_file_name,
                               const char* normals_file_name) {
  bool success = true;

  if (image_file_name != nullptr)
    success = dst_image_buffer_.SaveToFile(image_file_name);

  if (normals_file_name) {
    auto depth_image = dst_depth_buffer_.ComputeNormals();
    success = depth_image->SaveToFile(normals_file_name) && success;
    delete depth_image;
  }

  return success;
}

void DeepSurface::Transfer(bool and_clear) {
  int nx = src_depth_buffer_.GetWidth();
  int stride = src_depth_buffer_.GetStride();
  int new_line = stride - nx;
  int ny = src_depth_buffer_.GetHeight();

  auto dsrc = src_depth_buffer_.GetPtr();
  auto isrc = src_image_buffer_.GetPtr();
  auto ddst = dst_depth_buffer_.GetPtr();
  auto idst = dst_image_buffer_.GetPtr();

  size_t offset = 0U;
  for (int iy = 0; iy < ny; iy++, offset += new_line) {
    size_t final_line_offset = offset + nx;
    for (; offset < final_line_offset; offset++) {
      uint32_t src_argb = isrc[offset];
      uint8_t src_alpha = GetA(src_argb);
      if (src_alpha == 0)
        // Source is transparent. Nothing to do.
        continue;

      auto src_depth = dsrc[offset];
      if (DepthBuffer::IsInfiniteDepth(src_depth))
        // Source has infinite depth. Again, nothing to do.
        continue;

      auto dst_depth = ddst[offset];
      if (DepthBuffer::IsInfiniteDepth(dst_depth) || src_depth >= dst_depth) {
        ddst[offset] = src_depth;
        if (src_alpha == 0xff)
          idst[offset] = src_argb;
        else
          idst[offset] = BlendSrcOverDst(src_argb, idst[offset]);
      } else {
        assert(src_depth < dst_depth);
        idst[offset] = BlendSrcOverDst(idst[offset], src_argb);
      }
    }
  }

  if (and_clear)
    Clear();
}

#ifdef TEST_DEPTH_SURFACE

// Build as a standalone C++ program for testing.
int main() {
  DeepSurface ds(320, 320, 200);

  auto ib = ds.GetSrcImageBuffer();
  auto db = ds.GetSrcDepthBuffer();

  ib->DrawCircle(107, 70, 80, 0xffff0000);
  db->DrawSphere(107, 70, 0, 80, 1.0);
  ds.Transfer();

  ib->DrawCircle(214, 130, 80, 0xffffff00);
  db->DrawSphere(214, 130, 10, 80, 1.0);
  ds.Transfer();

  ds.SaveToFiles("spheres-image.png", "spheres-normals.png");
}
#endif
