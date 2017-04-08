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

#include "mesh.h"
#include "depth_buffer.h"

#include <assert.h>
#include <memory>
#include <istream>
#include <limits>
#include <fstream>
#include <iostream>
#include <sstream>

namespace deepsurface {

void Mesh::Draw(DepthBuffer* db, ARGBImageBuffer* ib,
                const Affine3<float>& mx) {
  for (size_t zone_idx = 0; zone_idx < tex_zones_.size(); zone_idx++) {
    auto&& tex_zone = tex_zones_[zone_idx];
    Texture* cur_texture = tex_zone.texture.get();
    size_t begin_idx = tex_zone.starting_face;
    size_t end_idx = faces_.size();
    if (zone_idx + 1 < tex_zones_.size())
      end_idx = tex_zones_[zone_idx + 1].starting_face;

    for (size_t face_idx = begin_idx; face_idx < end_idx; face_idx++) {
      auto&& face = faces_[face_idx];
      Point3 p[face.indices.size()];
      Point2 tex_center{0.0f, 0.0f};
      for (int idx = 0; idx < face.size; idx++) {
        uint32_t v_idx = static_cast<uint32_t>(face.indices[idx].vertex) - 1U;
        uint32_t t_idx = static_cast<uint32_t>(face.indices[idx].tex) - 1U;
        if (t_idx >= tex_coords_.size() ||
            v_idx >= vertices_.size()) {
          std::cout << "ERROR IN Mesh::Draw()" << std::endl;
          return;
        }
        tex_center[0] += tex_coords_[t_idx][0];
        tex_center[1] += tex_coords_[t_idx][1];
        p[idx] = mx.Apply(vertices_[v_idx]);
      }

      tex_center[0] /= face.size;
      tex_center[1] /= face.size;
      uint32_t color = cur_texture->ColorAt(tex_center[0], tex_center[1]);
      DrawTriangle(db, ib, p[0], p[1], p[2], color);
      if (face.indices.size() == 4)
        DrawTriangle(db, ib, p[2], p[3], p[0], color);
    }
  }
}

template <typename Scalar>
static bool InvertMatrix(Scalar* mx_out, Scalar* mx_in) {
  Scalar* m0 = &mx_in[0];
  Scalar* m1 = &mx_in[3];
  Scalar det = m0[0]*m1[1] - m0[1]*m1[0];
  if (det == 0.0)
    return false;

  Scalar* o0 = &mx_out[0];
  Scalar* o1 = &mx_out[3];
  o0[0] =  m1[1]/det; o0[1] = -m0[1]/det;
  o1[0] = -m1[0]/det; o1[1] =  m0[0]/det;
  o0[2] = -o0[0]*m0[2] - o0[1]*m1[2];
  o1[2] = -o1[0]*m0[2] - o1[1]*m1[2];
  return true;
}

template<typename DepthType, typename PixelType, typename DepthFn>
void DrawDepth(ImageBuffer<DepthType>* depth_buffer,
               ImageBuffer<PixelType>* image_buffer,
               float clip_start_x, float clip_start_y,
               float clip_end_x, float clip_end_y,
               float* matrix, DepthFn depth_fn) {
  if (clip_end_x < clip_start_x) std::swap(clip_start_x, clip_end_x);
  if (clip_end_y < clip_start_y) std::swap(clip_start_y, clip_end_y);
  int ix0 = static_cast<int>(floorf(clip_start_x));
  int iy0 = static_cast<int>(floorf(clip_start_y));
  int ix1 = static_cast<int>(ceilf(clip_end_x));
  int iy1 = static_cast<int>(ceilf(clip_end_y));
  auto depth_region = depth_buffer->GetRegion(ix0, iy0, ix1, iy1);
  auto image_region = image_buffer->GetRegion(ix0, iy0, ix1, iy1);
  if (!(depth_region.IsValid() && image_region.IsValid())) {
    return;
  }

  float u_at_line_start = matrix[0]*ix0 + matrix[1]*iy0 + matrix[2];
  float v_at_line_start = matrix[3]*ix0 + matrix[4]*iy0 + matrix[5];
  float u_increment_per_pixel = matrix[0];
  float u_increment_per_line = matrix[1];
  float v_increment_per_pixel = matrix[3];
  float v_increment_per_line = matrix[4];

  int32_t width = depth_region.GetWidth();
  int32_t stride = depth_region.GetStride();
  int32_t height = depth_region.GetHeight();
  assert(image_region.GetWidth() == width);
  assert(image_region.GetWidth() == stride);
  assert(image_region.GetHeight() == height);

  int32_t skip = stride - width;
  DepthType* depth_ptr = depth_region.GetPtr();
  PixelType* image_ptr = image_region.GetPtr();
  DepthType* end_depth_ptr = depth_ptr + stride*height;
  DepthType* end_line;
  while ((end_line = depth_ptr + width) < end_depth_ptr) {
    float u = u_at_line_start;
    float v = v_at_line_start;
    while (depth_ptr < end_line) {
      depth_fn(depth_ptr, image_ptr, u, v);
      depth_ptr++;
      image_ptr++;
      u += u_increment_per_pixel;
      v += v_increment_per_pixel;
    }
    depth_ptr += skip;
    image_ptr += skip;
    u_at_line_start += u_increment_per_line;
    v_at_line_start += v_increment_per_line;
  }
}

static void DrawTriangleRaw(DepthBuffer* db, ARGBImageBuffer* ib,
                            float clip_start_x, float clip_start_y,
                            float clip_end_x, float clip_end_y,
                            float* mx, float z00, float z10, float z01,
                            //Texture* texture
                            uint32_t color) {
  //uint32_t color = (texture) ? texture->ColorAt(0.0f, 0.0f) : 0xffffffff;

  z01 -= z00;
  z10 -= z00;
  auto depth_fn =
    [z00, z10, z01, color](float* depth_out, uint32_t* image_out,
                           float u, float v) -> void {
      if (u >= 0.0 && v >= 0.0 && u + v <= 1.0) {
        float bg = *depth_out;
        float candidate = z00 + u*z01 + v*z10;
        if (DepthBuffer::IsInfiniteDepth(bg) || candidate > bg) {
          *depth_out = candidate;
          *image_out = BlendSrcOverDst(color, *image_out);
        }
      }
    };
  DrawDepth(db, ib, clip_start_x, clip_start_y, clip_end_x, clip_end_y,
            mx, depth_fn);
}

void Mesh::DrawTriangle(DepthBuffer* db, ARGBImageBuffer* ib,
                        const Point3& p00, const Point3& p10, const Point3& p01,
                        //Texture* texture
                        uint32_t color) {
  float mx[6] = {p10[0] - p00[0], p01[0] - p00[0], p00[0],
                 p10[1] - p00[1], p01[1] - p00[1], p00[1]};
  float inv_mx[6];
  if (InvertMatrix(inv_mx, mx)) {
    float clip_start_x = std::min(std::min(p00[0], p10[0]), p01[0]);
    float clip_start_y = std::min(std::min(p00[1], p10[1]), p01[1]);
    float clip_end_x   = std::max(std::max(p00[0], p10[0]), p01[0]);
    float clip_end_y   = std::max(std::max(p00[1], p10[1]), p01[1]);
    DrawTriangleRaw(db, ib, clip_start_x, clip_start_y, clip_end_x, clip_end_y,
                    inv_mx, p00[2], p01[2], p10[2], color);
  }
}

}  // namespace deepsurface
