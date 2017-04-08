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
#include "draw_depth.h"

#include <assert.h>
#include <memory>
#include <istream>
#include <limits>
#include <fstream>
#include <iostream>
#include <sstream>

namespace deepsurface {


class Quad {
 public:
  using Scalar = Mesh::Scalar;
  using Point2 = Mesh::Point2;
  using Point3 = Mesh::Point3;

  Quad() : size_(0) { }

  void Append(const Point3& vertex, const Point2& tex_coords) {
    if (size_ == 4)
      throw "Cannot Append: Quad is full";
    vertices_[size_] = vertex;
    tex_coords_[size_] = tex_coords;
    size_++;
  }

  void Draw(DepthBuffer* db, ARGBImageBuffer* ib, Texture* texture) {
    if (size_ >= 3) {
      DrawTriangle(db, ib, 0, 1, 2, texture);
      if (size_ == 4)
        DrawTriangle(db, ib, 2, 3, 0, texture);
    }
  }

 private:
  void DrawTriangle(DepthBuffer* db, ARGBImageBuffer* ib,
                    int idx0, int idx1, int idx2, Texture* texture);

  Point3 vertices_[4];
  Point2 tex_coords_[4];
  int size_;
};


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
      Quad quad;

      for (unsigned int idx = 0; idx < face.size; idx++) {
        uint32_t v_idx = static_cast<uint32_t>(face.indices[idx].vertex) - 1U;
        uint32_t t_idx = static_cast<uint32_t>(face.indices[idx].tex) - 1U;
        if (t_idx >= tex_coords_.size() ||
            v_idx >= vertices_.size()) {
          std::cout << "ERROR IN Mesh::Draw()" << std::endl;
          return;
        }
        quad.Append(mx.Apply(vertices_[v_idx]), tex_coords_[t_idx]);
      }

      quad.Draw(db, ib, cur_texture);
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

void Quad::DrawTriangle(DepthBuffer* db, ARGBImageBuffer* ib,
                        int idx0, int idx1, int idx2, Texture* texture) {
  const Point3& p00 = vertices_[idx0];
  const Point3& p10 = vertices_[idx1];
  const Point3& p01 = vertices_[idx2];
  const Point2& t00 = tex_coords_[idx0];
  const Point2& t10 = tex_coords_[idx1];
  const Point2& t01 = tex_coords_[idx2];

  float mx[6] = {p10[0] - p00[0], p01[0] - p00[0], p00[0],
                 p10[1] - p00[1], p01[1] - p00[1], p00[1]};
  float inv_mx[6];
  if (InvertMatrix(inv_mx, mx)) {
    float clip_start_x = std::min(std::min(p00[0], p10[0]), p01[0]);
    float clip_start_y = std::min(std::min(p00[1], p10[1]), p01[1]);
    float clip_end_x   = std::max(std::max(p00[0], p10[0]), p01[0]);
    float clip_end_y   = std::max(std::max(p00[1], p10[1]), p01[1]);

    VectorAttribute<float, 5> attrs;
    attrs[0].Set(inv_mx[0], inv_mx[1], inv_mx[2]);                   // u
    attrs[1].Set(inv_mx[3], inv_mx[4], inv_mx[5]);                   // v
    attrs[2].Set(p10[2] - p00[2], p01[2] - p00[2], p00[2], inv_mx);  // z
    attrs[3].Set(t10[0] - t00[0], t01[0] - t00[0], t00[0], inv_mx);  // tex_u
    attrs[4].Set(t10[1] - t00[1], t01[1] - t00[1], t00[1], inv_mx);  // tex_v

    auto depth_fn =
      [texture](float* depth_out, uint32_t* image_out,
                const std::array<float, 5>& values) -> void {
        float u = values[0];
        float v = values[1];
        if (u >= 0.0 && v >= 0.0 && u + v <= 1.0) {
          float bg = *depth_out;
          float candidate = values[2];
          if (DepthBuffer::IsInfiniteDepth(bg) || candidate > bg) {
            float tex_u = values[3];
            float tex_v = values[4];
            uint32_t color = texture->ColorAt(tex_u, tex_v);
            *depth_out = candidate;
            *image_out = BlendSrcOverDst(color, *image_out);
          }
        }
    };
    DrawDepth(db, ib, clip_start_x, clip_start_y, clip_end_x, clip_end_y,
              attrs, depth_fn);
  }
}

}  // namespace deepsurface
