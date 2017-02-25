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

#include <memory>
#include <istream>
#include <limits>
#include <fstream>
#include <iostream>
#include <sstream>

namespace deepsurface {

void Mesh::Draw(DepthBuffer* db, ARGBImageBuffer* ib,
                const Affine3<float>& mx) {
  for (auto&& face: faces_) {
    Point3 p[face.indices.size()];
    for (int idx = 0; idx < face.size; idx++) {
      uint32_t v_idx = static_cast<uint32_t>(face.indices[idx].vertex) - 1U;
      if (v_idx >= vertices_.size()) {
        std::cout << "ERROR IN Mesh::Draw()" << std::endl;
        return;
      }
      p[idx] = mx.Apply(vertices_[v_idx]);
    }
    DrawTriangle(db, ib, p[0], p[1], p[2]);
    if (face.indices.size() == 4)
      DrawTriangle(db, ib, p[2], p[3], p[0]);
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

void Mesh::DrawTriangle(DepthBuffer* db, ARGBImageBuffer* ib,
                        const Point3& p00, const Point3& p10,
                        const Point3& p01) {
  float mx[6] = {p10[0] - p00[0], p01[0] - p00[0], p00[0],
                 p10[1] - p00[1], p01[1] - p00[1], p00[1]};
  float inv_mx[6];
  if (InvertMatrix(inv_mx, mx)) {
    float clip_start_x = std::min(std::min(p00[0], p10[0]), p01[0]);
    float clip_start_y = std::min(std::min(p00[1], p10[1]), p01[1]);
    float clip_end_x   = std::max(std::max(p00[0], p10[0]), p01[0]);
    float clip_end_y   = std::max(std::max(p00[1], p10[1]), p01[1]);
    db->DrawTriangle(clip_start_x, clip_start_y,
                     clip_end_x, clip_end_y,
                     inv_mx, p00[2], p01[2], p10[2]);
  }
}

}  // namespace deepsurface
