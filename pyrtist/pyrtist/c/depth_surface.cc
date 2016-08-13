#include "depth_surface.h"

bool DepthSurface::SaveToFiles(const char* image_file_name,
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

void DepthSurface::Transfer(bool and_clear) {
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
      // Source is transparent. Nothing to do.
      uint32_t src_argb = isrc[offset];
      uint8_t src_alpha = src_argb >> 24;
      if (src_alpha == 0)
        continue;

      // Source has infinite depth. Again, nothing to do.
      auto src_depth = dsrc[offset];
      if (DepthBuffer::IsInfiniteDepth(src_depth))
        continue;

      auto dst_depth = ddst[offset];
      if (DepthBuffer::IsInfiniteDepth(dst_depth) || src_depth > dst_depth) {
        ddst[offset] = src_depth;
        idst[offset] = src_argb;
      } else if (src_depth < dst_depth) {
      } else {
      }
      //uint32_t dst_argb = idst[offset];
    }
  }

  if (and_clear)
    Clear();
}

int main() {
  DepthSurface ds(320, 320, 200);

  auto ib = ds.GetImageBuffer();
  auto db = ds.GetDepthBuffer();

  ib->DrawCircle(107, 70, 80, 0xffff0000);
  db->DrawSphere(107, 70, 0, 80, 1.0);
  ds.Transfer();

  ib->DrawCircle(214, 130, 80, 0xffffff00);
  db->DrawSphere(214, 130, 10, 80, 1.0);
  ds.Transfer();

  ds.SaveToFiles("spheres-image.png", "spheres-normals.png");
}
