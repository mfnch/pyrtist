// Copyright (C) 2017 Matteo Franchin
//
// This file is part of Pyrtist.
//   Pyrtist is free software: you can redistribute it and/or modify it
//   under the terms of the GNU Lesser General Public License as published
//   by the Free Software Foundation, either version 2.1 of the License, or
//   (at your option) any later version.
//
//   Pyrtist is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU Lesser General Public License for more details.
//
//   You should have received a copy of the GNU Lesser General Public License
//   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

#ifndef _DRAW_DEPTH_H
#define _DRAW_DEPTH_H

#include <algorithm>
#include <math.h>
#include <assert.h>

#include "image_buffer.h"

template<typename PixelType, typename DepthFn>
void DrawDepth(ImageBuffer<PixelType>* buffer,
               float clip_start_x, float clip_start_y,
               float clip_end_x, float clip_end_y,
               float* matrix, DepthFn depth_fn) {
  if (clip_end_x < clip_start_x) std::swap(clip_start_x, clip_end_x);
  if (clip_end_y < clip_start_y) std::swap(clip_start_y, clip_end_y);
  int ix0 = static_cast<int>(floorf(clip_start_x));
  int iy0 = static_cast<int>(floorf(clip_start_y));
  int ix1 = static_cast<int>(ceilf(clip_end_x));
  int iy1 = static_cast<int>(ceilf(clip_end_y));
  auto region = buffer->GetRegion(&ix0, &iy0, ix1, iy1);
  if (!region.IsValid())
    return;

  float u_at_line_start = matrix[0]*ix0 + matrix[1]*iy0 + matrix[2];
  float v_at_line_start = matrix[3]*ix0 + matrix[4]*iy0 + matrix[5];
  float u_increment_per_pixel = matrix[0];
  float u_increment_per_line = matrix[1];
  float v_increment_per_pixel = matrix[3];
  float v_increment_per_line = matrix[4];

  int32_t width = region.GetWidth();
  int32_t stride = region.GetStride();
  int32_t height = region.GetHeight();
  int32_t skip = stride - width;
  float* ptr = region.GetPtr();
  float* end_ptr = ptr + stride*height;
  PixelType* end_line;
  while ((end_line = ptr + width) < end_ptr) {
    float u = u_at_line_start;
    float v = v_at_line_start;
    while (ptr < end_line) {
      depth_fn(ptr, u, v);
      ptr++;
      u += u_increment_per_pixel;
      v += v_increment_per_pixel;
    }
    ptr += skip;
    u_at_line_start += u_increment_per_line;
    v_at_line_start += v_increment_per_line;
  }
}

namespace deepsurface {

/// @brief A linearly changing scalar field over a 2D lattice.
/// @details This object allows to compute a linear scalar field defined over
///   a 2-dimensional lattice. The nodes of the lattice are identified using a
///   pair of indices `ix, iy`. This object allows to retrieve the value of the
///   vector at a given node `ix0, iy0`. It then allows incrementing this value
///   to obtain the value of the scalar field at neighbouring nodes. The value
///   of the scalar field is just a number of type `Scalar`.
template <typename Scalar>
class ScalarAttribute {
 public:
  ScalarAttribute() {  }

  /// @brief Set the coefficient of the linear mapping.
  /// @details The scalar field is calculated from the integer tuple (ix, iy)
  ///   as c0*ix + c1+iy + c2. The three coefficient c0, c1, and c2 can be set
  ///   by calling this function.
  void Set(Scalar c0, Scalar c1, Scalar c2) {
    inc_x_ = c0; inc_y_ = c1; value_at_00_ = c2;
  }

  void Set(Scalar c0, Scalar c1, Scalar c2, Scalar (&matrix)[6]) {
    Scalar t0 = c0*matrix[0] + c1*matrix[3];
    Scalar t1 = c0*matrix[1] + c1*matrix[4];
    Scalar t2 = c0*matrix[2] + c1*matrix[5] + c2;
    Set(t0, t1, t2);
  }

  /// @brief Obtain the value of the scalar field at the given position.
  Scalar ValueAt(int ix, int iy) const {
    return inc_x_ * ix + inc_y_ * iy + value_at_00_;
  }

  /// @brief Increment the given scalar field value to obtain the value at the
  ///   next node in the x direction.
  void IncX(Scalar& v) const { v += inc_x_; }

  /// @brief Increment the given scalar field value to obtain the value at the
  ///   next node in the y direction.
  void IncY(Scalar& v) const { v += inc_y_; }

 private:
  Scalar inc_x_;
  Scalar inc_y_;
  Scalar value_at_00_;
};

/// @brief A linearly changing vector field over a 2D lattice.
/// @details This is object similar to ScalarAttribute, except that it computes
///   a vector (an array of Scalars) over a 2D lattice.
/// @see ScalarAttribute
template <typename Scalar, int N>
class VectorAttribute {
 public:
  using Value = std::array<Scalar, N>;

  VectorAttribute() { }

  /// @brief Obtain the value of the vector field at the given position.
  Value ValueAt(int ix, int iy) const {
    Value ret;
    for (size_t i = 0; i < attrs_.size(); i++)
      ret[i] = attrs_[i].ValueAt(ix, iy);
    return std::move(ret);
  }

  /// @brief Increment the given vector field value to obtain the value at the
  ///   next node in the x direction.
  void IncX(Value& value) const {
    for (size_t i = 0; i < attrs_.size(); i++)
      attrs_[i].IncX(value[i]);
  }

  /// @brief Increment the given vector field value to obtain the value at the
  ///   next node in the y direction.
  void IncY(Value& value) const {
    for (size_t i = 0; i < attrs_.size(); i++)
      attrs_[i].IncY(value[i]);
  }

  ScalarAttribute<Scalar>& operator[](int idx) { return attrs_[idx]; }

 private:
  std::array<ScalarAttribute<Scalar>, N> attrs_;
};

/// @brief Draw a primitive to a pair (depth-buffer, image-buffer).
template<typename Attrs,
         typename DepthType, typename PixelType, typename DepthFn>
void DrawDepth(ImageBuffer<DepthType>* depth_buffer,
               ImageBuffer<PixelType>* image_buffer,
               float clip_start_x, float clip_start_y,
               float clip_end_x, float clip_end_y,
               const Attrs& attrs, DepthFn depth_fn) {
  if (clip_end_x < clip_start_x) std::swap(clip_start_x, clip_end_x);
  if (clip_end_y < clip_start_y) std::swap(clip_start_y, clip_end_y);
  int ix0 = static_cast<int>(floorf(clip_start_x));
  int iy0 = static_cast<int>(floorf(clip_start_y));
  int ix1 = static_cast<int>(ceilf(clip_end_x));
  int iy1 = static_cast<int>(ceilf(clip_end_y));
  auto depth_region = depth_buffer->GetRegion(&ix0, &iy0, ix1, iy1);
  auto image_region = image_buffer->GetRegion(&ix0, &iy0, ix1, iy1);
  if (!(depth_region.IsValid() && image_region.IsValid()))
    return;

  auto values_at_line_start = attrs.ValueAt(ix0, iy0);
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
    auto values = values_at_line_start;
    while (depth_ptr < end_line) {
      depth_fn(depth_ptr, image_ptr, values);
      depth_ptr++;
      image_ptr++;
      attrs.IncX(values);
    }
    depth_ptr += skip;
    image_ptr += skip;
    attrs.IncY(values_at_line_start);
  }
}

}  // namespace deepsurface

#endif  /* _DRAW_DEPTH_H */
